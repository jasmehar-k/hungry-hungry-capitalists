#include "player.h"

#include <utility>

#include "boarddata.h"

Player::Player(std::string name, char token)
    : name_{std::move(name)}, token_{token} {}

// Being sent to The Lockup starts the count of turns served at zero, whether
// the Artificer arrived by the square at 30, by a Wandering Market or by
// rolling three consecutive doubles.
void Player::sendToLockup() {
    position_ = kLockupSquare;
    inLockup_ = true;
    lockupTurns_ = 0;
}

// Leaving clears the count as well, so an Artificer detained a second time
// later in the game serves three fresh turns rather than resuming an old
// count.
void Player::releaseFromLockup() {
    inLockup_ = false;
    lockupTurns_ = 0;
}

// Restores the state verbatim from a save file, where square 10 carries a
// trailing flag and, if detained, the number of turns already served. This is
// separate from sendToLockup because loading must not reset that count.
void Player::setLockupState(bool detained, int turnsServed) {
    inLockup_ = detained;
    lockupTurns_ = detained ? turnsServed : 0;
}
