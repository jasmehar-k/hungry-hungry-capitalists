#include "depot.h"

#include <utility>

#include "dice.h"
#include "game.h"
#include "player.h"

Depot::Depot(int square, std::string name, int buyPrice)
    : OwnableDistrict{square, std::move(name), buyPrice} {}

// The only toll in the game that depends on the dice rather than on the
// district alone, which is why Depot reaches for the Game's dice here.
int Depot::computeToll(const Game &game, const Player &) const {
    if (owner() == nullptr) return 0;
    // Priced off the roll that brought the visitor here, not off a fixed table.
    const int held = game.countOwned(DistrictKind::Depot, *owner());
    const int multiplier = held >= 2 ? kDepotBothMultiplier : kDepotSingleMultiplier;
    return multiplier * game.dice().total();
}
