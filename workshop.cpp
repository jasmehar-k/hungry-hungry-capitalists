#include "workshop.h"

#include <utility>

Workshop::Workshop(int square, std::string name, Guild guild, int buyPrice, int upgradeCost,
                   const std::array<int, 6> &tolls)
    : OwnableDistrict{square, std::move(name), buyPrice},
      guild_{guild},
      upgradeCost_{upgradeCost},
      tolls_{tolls} {}

int Workshop::tollAt(int level) const {
    return tolls_[static_cast<std::size_t>(level)];
}

int Workshop::computeToll(const Game &, const Player &) const {
    // TODO(movement step): double this when the owner holds every Workshop in
    // the Guild and this one is unimproved. Guild sizes differ, so that check
    // must count the Guild's actual members rather than assume three.
    return tollAt(upgrades_);
}
