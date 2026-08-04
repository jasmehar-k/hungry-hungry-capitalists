#ifndef PLAYER_H
#define PLAYER_H

#include <cstddef>
#include <string>

inline constexpr int kStartingReagents = 1500;
inline constexpr int kMaxLockupTurns = 3;

// The display cannot fit more than six tokens in a district cell, which is why
// the spec caps play at six Artificers.
inline constexpr std::size_t kMinPlayers = 2;
inline constexpr std::size_t kMaxPlayers = 6;

class Player {
public:
    Player(std::string name, char token);

    const std::string &name() const { return name_; }
    char token() const { return token_; }

    int reagents() const { return reagents_; }
    void addReagents(int amount) { reagents_ += amount; }

    int position() const { return position_; }
    void setPosition(int square) { position_ = square; }

    int freePasses() const { return freePasses_; }
    void setFreePasses(int count) { freePasses_ = count; }

    bool inLockup() const { return inLockup_; }
    int lockupTurns() const { return lockupTurns_; }
    void sendToLockup();
    void releaseFromLockup();
    void serveLockupTurn() { ++lockupTurns_; }

private:
    std::string name_;
    char token_;
    int reagents_ = kStartingReagents;
    int position_ = 0;
    int freePasses_ = 0;
    bool inLockup_ = false;
    int lockupTurns_ = 0;
};

#endif  // PLAYER_H
