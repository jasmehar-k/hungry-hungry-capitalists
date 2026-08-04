#include "depot.h"

#include <utility>

Depot::Depot(int square, std::string name, int buyPrice)
    : OwnableDistrict{square, std::move(name), buyPrice} {}

int Depot::computeToll(const Game &, const Player &) const {
    // TODO(movement step): 4x the dice total, or 10x when the owner holds both
    // Depots. Needs the roll that brought the visitor here.
    return 0;
}
