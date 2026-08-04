#ifndef PLAYER_H
#define PLAYER_H

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

inline constexpr int kStartingReagents = 1500;
inline constexpr int kMaxLockupTurns = 3;

// The display cannot fit more than six tokens in a district cell, which is why
// the spec caps play at six Artificers.
inline constexpr std::size_t kMinPlayers = 2;
inline constexpr std::size_t kMaxPlayers = 6;

// The six tokens an Artificer may choose; only one Artificer may hold each.
struct TokenChoice {
    char symbol;
    std::string_view name;
};

inline constexpr std::array<TokenChoice, kMaxPlayers> kTokens{{{'G', "Gryphon"},
                                                              {'O', "Golem"},
                                                              {'X', "Sphinx"},
                                                              {'W', "Wyvern"},
                                                              {'A', "Archmage"},
                                                              {'P', "Apprentice"}}};

inline constexpr bool isValidToken(char symbol) {
    for (const TokenChoice &choice : kTokens) {
        if (choice.symbol == symbol) return true;
    }
    return false;
}

class Player {
public:
    Player(std::string name, char token);

    // An entity, not a value: districts hold Player* to record ownership, so
    // copying one would silently split its identity.
    Player(const Player &) = delete;
    Player &operator=(const Player &) = delete;

    const std::string &name() const { return name_; }
    char token() const { return token_; }

    int reagents() const { return reagents_; }
    void addReagents(int amount) { reagents_ += amount; }
    void setReagents(int amount) { reagents_ = amount; }

    int position() const { return position_; }
    void setPosition(int square) { position_ = square; }

    int freePasses() const { return freePasses_; }
    void setFreePasses(int count) { freePasses_ = count; }

    // An insolvent Artificer is out of the game but stays in the roster so
    // that districts loaded under their name still resolve.
    bool isInsolvent() const { return insolvent_; }
    void declareInsolvent() { insolvent_ = true; }

    bool inLockup() const { return inLockup_; }
    int lockupTurns() const { return lockupTurns_; }
    void sendToLockup();
    void releaseFromLockup();
    void serveLockupTurn() { ++lockupTurns_; }
    // Restores Lockup state verbatim when loading a saved game.
    void setLockupState(bool detained, int turnsServed);

private:
    std::string name_;
    char token_;
    int reagents_ = kStartingReagents;
    int position_ = 0;
    int freePasses_ = 0;
    bool inLockup_ = false;
    int lockupTurns_ = 0;
    bool insolvent_ = false;
};

#endif  // PLAYER_H
