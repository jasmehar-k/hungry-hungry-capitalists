#include "player.h"

#include <utility>

#include "boarddata.h"

Player::Player(std::string name, char token)
    : name_{std::move(name)}, token_{token} {}

void Player::sendToLockup() {
    position_ = kLockupSquare;
    inLockup_ = true;
    lockupTurns_ = 0;
}

void Player::releaseFromLockup() {
    inLockup_ = false;
    lockupTurns_ = 0;
}
