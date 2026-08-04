#include "forge.h"

#include <utility>

#include "game.h"
#include "player.h"

Forge::Forge(int square, std::string name, int buyPrice)
    : OwnableDistrict{square, std::move(name), buyPrice} {}

int Forge::computeToll(const Game &game, const Player &) const {
    if (owner() == nullptr) return 0;
    // The levy depends on how many Forges the owner controls, not on this one.
    const int held = game.countOwned(DistrictKind::Forge, *owner());
    if (held <= 0 || static_cast<std::size_t>(held) >= kForgeLevy.size()) return 0;
    return kForgeLevy[static_cast<std::size_t>(held)];
}
