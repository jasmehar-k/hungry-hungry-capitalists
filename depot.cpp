#include "depot.h"

#include <utility>

#include "dice.h"
#include "game.h"
#include "player.h"

Depot::Depot(int square, std::string name, int buyPrice)
    : OwnableDistrict{square, std::move(name), buyPrice} {}

int Depot::computeToll(const Game &game, const Player &) const {
    if (owner() == nullptr) return 0;
    // Priced off the roll that brought the visitor here, not off a fixed table.
    const int held = game.countOwned(DistrictKind::Depot, *owner());
    const int multiplier = held >= 2 ? kDepotBothMultiplier : kDepotSingleMultiplier;
    return multiplier * game.dice().total();
}
