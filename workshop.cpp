#include "workshop.h"

#include <utility>

#include "game.h"
#include "player.h"

Workshop::Workshop(int square, std::string name, Guild guild, int buyPrice, int upgradeCost,
                   const std::array<int, 6> &tolls)
    : OwnableDistrict{square, std::move(name), buyPrice},
      guild_{guild},
      upgradeCost_{upgradeCost},
      tolls_{tolls} {}

// The level indexes the table directly: entry 0 is the unimproved toll and
// entries 1 to 5 are the upgrade levels, which is why the array has six slots
// for five upgrades.
int Workshop::tollAt(int level) const {
    return tolls_[static_cast<std::size_t>(level)];
}

int Workshop::computeToll(const Game &game, const Player &) const {
    const int base = tollAt(upgrades_);
    // A fully-owned Guild doubles the toll on its *unimproved* Workshops only;
    // once upgraded, the table already carries the higher figure.
    if (upgrades_ == 0 && owner() != nullptr && game.guildComplete(guild_, *owner())) {
        return base * 2;
    }
    return base;
}
