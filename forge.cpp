#include "forge.h"

#include <utility>

Forge::Forge(int square, std::string name, int buyPrice)
    : OwnableDistrict{square, std::move(name), buyPrice} {}

int Forge::computeToll(const Game &, const Player &) const {
    // TODO(movement step): kForgeLevy indexed by how many Forges the owner holds.
    return 0;
}
