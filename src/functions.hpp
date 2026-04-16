#ifndef FUNCTIONS5CARD_HPP
#define FUNCTIONS5CARD_HPP
 
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <fstream>
#include <thread>
#include <chrono>
#include <conio.h>
#include <map>
#include <set>
 
// ANSI Colors
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define CYAN    "\033[36m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"
#define GREEN   "\033[32m"
#define MAGENTA "\033[35m"
 
// --- GLOBAL ECONOMY ---
inline int playerBalance = 1000;
inline int currentBet = 0;
 
enum class Rank   { TWO = 2, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, JACK, QUEEN, KING, ACE };
enum class Symbol { SPADES = 1, CLUBS = 2, DIAMONDS = 3, HEART = 4 };
 
enum HandValue {
    HIGH_CARD = 1, PAIR = 2, TWO_PAIR = 3, THREE_KIND = 4,
    STRAIGHT = 5, FLUSH = 6, FULL_HOUSE = 7, FOUR_KIND = 8,
    STRAIGHT_FLUSH = 9, ROYAL_FLUSH = 10
};
 
struct HandResult {
    HandValue value;
    int       totalScore;
    std::string name;
};
 
struct Card {
    Rank   rank;
    Symbol symbol;
    void print() const {
        std::string r[] = {"","","2","3","4","5","6","7","8","9","10","J","Q","K","A"};
        std::string s[] = {"","S","C","D","H"};
        if (symbol == Symbol::HEART || symbol == Symbol::DIAMONDS) std::cout << RED;
        else std::cout << CYAN;
        std::cout << "[" << r[static_cast<int>(rank)] << s[static_cast<int>(symbol)] << "]" << RESET << " ";
    }
};
 
// --- BETTING ---
inline void placeBet() {
    if (playerBalance <= 0) {
        std::cout << RED << "\n[!] You're broke! The dealer grants you $200 charity.\n" << RESET;
        playerBalance = 200;
    }
    while (true) {
        std::cout << GREEN << BOLD << "\nCURRENT BALANCE: $" << playerBalance << RESET << "\n";
        std::cout << "Enter your bet amount: $";
        if (!(std::cin >> currentBet)) {
            std::cin.clear(); std::cin.ignore(1000, '\n');
            std::cout << RED << "Invalid input. Enter a number." << RESET << "\n";
        } else if (currentBet <= 0) {
            std::cout << RED << "Bet must be greater than 0." << RESET << "\n";
        } else if (currentBet > playerBalance) {
            std::cout << RED << "Insufficient funds!" << RESET << "\n";
        } else { break; }
    }
    std::cin.ignore(1000, '\n');
}
 
inline std::string getHandName(HandValue v) {
    switch (v) {
        case HIGH_CARD:     return "HIGH CARD";
        case PAIR:          return "ONE PAIR";
        case TWO_PAIR:      return "TWO PAIR";
        case THREE_KIND:    return "THREE OF A KIND";
        case STRAIGHT:      return "STRAIGHT";
        case FLUSH:         return "FLUSH";
        case FULL_HOUSE:    return "FULL HOUSE";
        case FOUR_KIND:     return "FOUR OF A KIND";
        case STRAIGHT_FLUSH:return "STRAIGHT FLUSH";
        case ROYAL_FLUSH:   return "ROYAL FLUSH";
        default:            return "UNKNOWN";
    }
}
 
// Evaluates a 5-card hand exactly
inline HandResult evaluateHand(const std::vector<Card>& hand) {
    // hand must be exactly 5 cards
    std::vector<Card> h = hand;
    std::sort(h.begin(), h.end(), [](const Card& a, const Card& b){ return a.rank < b.rank; });
 
    std::map<Rank, int> rankCounts;
    std::map<Symbol, int> suitCounts;
    for (const auto& c : h) { rankCounts[c.rank]++; suitCounts[c.symbol]++; }
 
    bool isFlush = false;
    for (auto const& [suit, cnt] : suitCounts) if (cnt == 5) isFlush = true;
 
    std::vector<int> ranks;
    for (const auto& c : h) ranks.push_back(static_cast<int>(c.rank));
 
    bool isStraight = false;
    int  straightHigh = 0;
    if ((ranks[4] - ranks[0] == 4) &&
        (std::set<int>(ranks.begin(), ranks.end()).size() == 5)) {
        isStraight = true; straightHigh = ranks[4];
    }
    // Ace-low straight: A-2-3-4-5
    if (ranks[4] == 14 && ranks[0] == 2 && ranks[1] == 3 && ranks[2] == 4 && ranks[3] == 5) {
        isStraight = true; straightHigh = 5;
    }
 
    int quads = 0, trips = 0, pairs = 0;
    int highRank = ranks[4]; // highest card rank
    for (auto const& [rank, count] : rankCounts) {
        if (count == 4) quads++;
        else if (count == 3) trips++;
        else if (count == 2) pairs++;
    }
 
    HandValue v = HIGH_CARD;
    if      (isFlush && isStraight && straightHigh == 14) v = ROYAL_FLUSH;
    else if (isFlush && isStraight)                       v = STRAIGHT_FLUSH;
    else if (quads > 0)                                   v = FOUR_KIND;
    else if (trips > 0 && pairs > 0)                      v = FULL_HOUSE;
    else if (isFlush)                                     v = FLUSH;
    else if (isStraight)                                  v = STRAIGHT;
    else if (trips > 0)                                   v = THREE_KIND;
    else if (pairs >= 2)                                  v = TWO_PAIR;
    else if (pairs == 1)                                  v = PAIR;
 
    return { v, (static_cast<int>(v) * 100) + highRank, getHandName(v) };
}
 
inline void clearScreen() { system("cls"); }
 
inline void printHeader() {
    std::cout << MAGENTA << BOLD << "========================================" << RESET << "\n";
    std::cout << MAGENTA << BOLD << "       5-CARD DRAW POKER ENGINE         " << RESET << "\n";
    std::cout << MAGENTA << BOLD << "========================================" << RESET << "\n";
}
 
inline void bootingSequence() {
    clearScreen();
    std::cout << CYAN << "[SYSTEM]: Initializing Audio & Cards..." << RESET << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
 
inline void waitForEnter() {
    std::cout << "\n" << YELLOW << "Press ENTER to return to menu..." << RESET;
    while (_getch() != 13);
}
 
inline void showFile(const std::string& filename) {
    clearScreen(); printHeader();
    std::ifstream file(filename);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) std::cout << line << "\n";
        file.close();
    } else {
        std::cout << RED << "File not found: " << filename << RESET << "\n";
    }
    waitForEnter();
}
 
// --- DECK ---
class Deck {
private:
    std::vector<Card> cards;
public:
    Deck() {
        for (int r = 2; r <= 14; ++r)
            for (int s = 1; s <= 4; ++s)
                cards.push_back({ static_cast<Rank>(r), static_cast<Symbol>(s) });
    }
    void shuffleDeck() {
        std::random_device rd; std::mt19937 g(rd());
        std::shuffle(cards.begin(), cards.end(), g);
    }
    Card drawCard() { Card c = cards.back(); cards.pop_back(); return c; }
};
 
// --- SIMPLE CPU AI: decide which cards to hold ---
// Strategy: keep pairs, trips, quads, flush/straight draws; discard the rest
inline std::vector<bool> cpuDecide(const std::vector<Card>& hand) {
    std::map<Rank, int> rc;
    std::map<Symbol, int> sc;
    for (const auto& c : hand) { rc[c.rank]++; sc[c.symbol]++; }
 
    bool flushDraw = false;
    for (auto& [s, cnt] : sc) if (cnt >= 3) flushDraw = true;
 
    std::vector<bool> keep(5, false);
    // Keep any card that is part of a pair or better
    for (int i = 0; i < 5; i++) {
        if (rc[hand[i].rank] >= 2) keep[i] = true;
    }
    // If keeping nothing, keep the highest card
    bool anyKept = false;
    for (bool b : keep) if (b) anyKept = true;
    if (!anyKept) {
        int maxIdx = 0;
        for (int i = 1; i < 5; i++)
            if (hand[i].rank > hand[maxIdx].rank) maxIdx = i;
        keep[maxIdx] = true;
    }
    // Flush draw: keep the 3+ suited cards
    if (!anyKept && flushDraw) {
        Symbol best = Symbol::SPADES; int bestCnt = 0;
        for (auto& [s, cnt] : sc) if (cnt > bestCnt) { best = s; bestCnt = cnt; }
        for (int i = 0; i < 5; i++)
            if (hand[i].symbol == best) keep[i] = true;
    }
    return keep;
}
 
#endif