#include "functions.hpp"
#include <windows.h>
#include "miniaudio.h"
#include <stdio.h>
#include <thread>
#include <chrono>
 
// -----------------------------------------------
// Print a hand with optional "held" indicators
// -----------------------------------------------
void printHand(const std::vector<Card>& hand, const std::vector<bool>& held = {}) {
    for (int i = 0; i < (int)hand.size(); i++) {
        hand[i].print();
    }
    std::cout << "\n";
    if (!held.empty()) {
        for (int i = 0; i < (int)hand.size(); i++) {
            std::string r[] = {"","","2","3","4","5","6","7","8","9","10","J","Q","K","A"};
            std::string s[] = {"","S","C","D","H"};
            std::string label = "[" + r[static_cast<int>(hand[i].rank)] + s[static_cast<int>(hand[i].symbol)] + "]";
            // Pad to match card width (min 4 chars + space)
            int pad = (int)label.size() < 5 ? 5 : (int)label.size();
            std::string marker = held[i] ? (GREEN + std::string("HOLD") + RESET) : "    ";
            std::cout << marker;
            if ((int)label.size() == 4) std::cout << " "; // extra space for [10x]
        }
        std::cout << "\n";
    }
}
 
// -----------------------------------------------
// Player draw phase: choose which cards to keep
// -----------------------------------------------
std::vector<bool> playerDrawPhase(std::vector<Card>& hand, Deck& deck) {
    std::vector<bool> held(5, false);
 
    while (true) {
        clearScreen();
        printHeader();
        std::cout << YELLOW << "Pot: $" << currentBet
                  << " | Balance: $" << playerBalance - currentBet << RESET << "\n\n";
        std::cout << BOLD << "YOUR HAND:\n" << RESET;
        std::cout << "  ";
        printHand(hand, held);
        std::cout << "\n  ";
        for (int i = 0; i < 5; i++) std::cout << "  " << (i + 1) << "   ";
        std::cout << "\n\n";
        std::cout << "Type card numbers to toggle HOLD (e.g. 1 3 5), then press ENTER.\n";
        std::cout << "Press ENTER with no input to confirm and draw.\n> ";
 
        std::string line;
        std::getline(std::cin, line);
 
        if (line.empty()) break; // confirm
 
        bool changed = false;
        for (char ch : line) {
            if (ch >= '1' && ch <= '5') {
                int idx = ch - '1';
                held[idx] = !held[idx];
                changed = true;
            }
        }
        (void)changed; // suppress unused warning
    }
 
    // Replace un-held cards
    for (int i = 0; i < 5; i++) {
        if (!held[i]) hand[i] = deck.drawCard();
    }
    return held;
}
 
// -----------------------------------------------
// CPU draw phase (simulated)
// -----------------------------------------------
int cpuDrawPhase(std::vector<Card>& cpuHand, Deck& deck) {
    std::vector<bool> keep = cpuDecide(cpuHand);
    int discarded = 0;
    for (int i = 0; i < 5; i++) {
        if (!keep[i]) { cpuHand[i] = deck.drawCard(); discarded++; }
    }
    return discarded;
}
 
// -----------------------------------------------
// Refresh UI (pre-draw phase)
// -----------------------------------------------
void refreshUI(const std::vector<Card>& pHand, bool showCpu = false,
               const std::vector<Card>& cpuHand = {}) {
    clearScreen();
    printHeader();
    std::cout << YELLOW << "Pot: $" << currentBet
              << " | Balance: $" << playerBalance - currentBet << RESET << "\n\n";
 
    std::cout << BOLD << "YOUR HAND:  " << RESET;
    printHand(pHand);
 
    std::cout << BOLD << "CPU HAND:   " << RESET;
    if (!showCpu) {
        std::cout << "[??] [??] [??] [??] [??]\n";
    } else {
        printHand(cpuHand);
    }
    std::cout << "----------------------------------------\n";
}
 
// -----------------------------------------------
// Main game loop
// -----------------------------------------------
void playGame(ma_engine* pMainEngine) {
    clearScreen();
    printHeader();
    placeBet();
 
    Deck myDeck;
    myDeck.shuffleDeck();
 
    // Deal 5 cards each
    std::vector<Card> playerHand, cpuHand;
    for (int i = 0; i < 5; i++) playerHand.push_back(myDeck.drawCard());
    for (int i = 0; i < 5; i++) cpuHand.push_back(myDeck.drawCard());
 
    // --- OPENING HAND ---
    refreshUI(playerHand);
    std::cout << "\nPress ANY KEY to begin the draw phase...";
    _getch();
 
    // --- PLAYER DRAW PHASE ---
    playerDrawPhase(playerHand, myDeck);
 
    // --- CPU DRAW PHASE ---
    clearScreen(); printHeader();
    std::cout << YELLOW << "CPU is thinking..." << RESET << "\n";
    int cpuDiscarded = cpuDrawPhase(cpuHand, myDeck);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << GREEN << "CPU drew " << cpuDiscarded << " card(s)." << RESET << "\n\n";
    std::cout << "Press ANY KEY for SHOWDOWN...";
    _getch();
 
    // --- SHOWDOWN ---
    HandResult playerResult = evaluateHand(playerHand);
    HandResult cpuResult    = evaluateHand(cpuHand);
 
    clearScreen();
    printHeader();
    std::cout << RED << BOLD << "\n*** SHOWDOWN ***" << RESET << "\n\n";
    std::cout << "YOUR HAND: "; printHand(playerHand);
    std::cout << " >> " << CYAN << playerResult.name << RESET << "\n\n";
    std::cout << "CPU HAND:  "; printHand(cpuHand);
    std::cout << " >> " << CYAN << cpuResult.name << RESET << "\n\n";
 
    // --- PAYOUT ---
    if (playerResult.totalScore > cpuResult.totalScore) {
        int winnings = currentBet * static_cast<int>(playerResult.value);
        playerBalance += winnings;
        std::cout << GREEN << BOLD << ">> YOU WIN! <<" << RESET << "\n";
        std::cout << "Hand Multiplier : " << static_cast<int>(playerResult.value) << "x\n";
        std::cout << "Profit          : $" << winnings << "\n";
 
        ma_engine_stop(pMainEngine);
        ma_result r3; ma_engine eng3;
        r3 = ma_engine_init(NULL, &eng3);
        if (r3 == MA_SUCCESS) {
            ma_engine_play_sound(&eng3, "jackpot.wav", NULL);
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ma_engine_uninit(&eng3);
        }
        ma_engine_start(pMainEngine);
    }
    else if (cpuResult.totalScore > playerResult.totalScore) {
        playerBalance -= currentBet;
        std::cout << RED << BOLD << ">> CPU WINS! <<" << RESET << "\n";
        std::cout << "Lost Bet : -$" << currentBet << "\n";
 
        ma_engine_stop(pMainEngine);
        ma_result r2; ma_engine eng2;
        r2 = ma_engine_init(NULL, &eng2);
        if (r2 == MA_SUCCESS) {
            ma_engine_play_sound(&eng2, "fart.wav", NULL);
            std::this_thread::sleep_for(std::chrono::seconds(2));
            ma_engine_uninit(&eng2);
        }
        ma_engine_start(pMainEngine);
    }
    else {
        std::cout << YELLOW << BOLD << ">> DRAW! Bet returned. <<" << RESET << "\n";
    }
 
    std::cout << BOLD << "\nNEW BALANCE: $" << playerBalance << RESET << "\n";
    waitForEnter();
}
 
// -----------------------------------------------
// Entry point
// -----------------------------------------------
int main() {
    bootingSequence();
 
    ma_result result1;
    ma_engine engine1;
    result1 = ma_engine_init(NULL, &engine1);
    if (result1 != MA_SUCCESS) return -1;
    ma_engine_play_sound(&engine1, "sound.wav", NULL);
 
    int choice = 0;
    std::string options[] = { "Start Game", "Rules", "Hand Rankings", "Quit" };
 
    while (true) {
        clearScreen();
        printHeader();
        std::cout << YELLOW << "BALANCE: $" << playerBalance << RESET << "\n\n";
        for (int i = 0; i < 4; i++) {
            if (i == choice) std::cout << GREEN << BOLD << "  > " << options[i] << " <" << RESET << "\n";
            else             std::cout << "    " << options[i] << "\n";
        }
 
        int key = _getch();
        if (key == 224) {
            key = _getch();
            if (key == 72) choice = (choice - 1 + 4) % 4;
            if (key == 80) choice = (choice + 1) % 4;
        } else if (key == 13) {
            if      (choice == 0) playGame(&engine1);
            else if (choice == 1) showFile(".files\\rules.txt");
            else if (choice == 2) showFile(".files\\points.txt");
            else if (choice == 3) break;
        }
    }
 
    ma_engine_uninit(&engine1);
    return 0;
}