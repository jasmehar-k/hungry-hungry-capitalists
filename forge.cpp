#include "forge.h"

#include <utility>

#include "game.h"
#include "player.h"

Forge::Forge(int square, std::string name, int buyPrice)
    : OwnableDistrict{square, std::move(name), buyPrice} {}

// The visitor is unused: what is owed depends only on how many Forges the
// owner holds. The parameter stays because the hook is shared with Depot,
// whose toll does depend on the roll that brought the visitor here.
int Forge::computeToll(const Game &game, const Player &) const {
    if (owner() == nullptr) return 0;
    // The levy depends on how many Forges the owner controls, not on this one.
    const int held = game.countOwned(DistrictKind::Forge, *owner());
    // Bounds-checked rather than trusted: the levy table is indexed by a count
    // that comes from a board sweep, and a stray index here would read past it.
    if (held <= 0 || static_cast<std::size_t>(held) >= kForgeLevy.size()) return 0;
    return kForgeLevy[static_cast<std::size_t>(held)];
}
