#include "district.h"

#include <iostream>
#include <utility>

#include "game.h"
#include "player.h"

District::District(int square, std::string name)
    : square_{square}, name_{std::move(name)} {}

OwnableDistrict::OwnableDistrict(int square, std::string name, int buyPrice)
    : District{square, std::move(name)}, buyPrice_{buyPrice} {}

char OwnableDistrict::ownerInitial() const {
    return owner_ ? owner_->token() : ' ';
}

// Template Method: the order of a visit is fixed here and cannot be overridden.
// Only computeToll varies between Workshops, Forges and Depots.
void OwnableDistrict::landOn(Game &game, Player &visitor) {
    if (owner_ == nullptr) {
        game.offerPurchase(*this, visitor);
        return;
    }
    if (owner_ == &visitor) return;  // an Artificer never tolls themselves
    if (mortgaged_) {
        std::cout << name() << " is mortgaged, so no Toll Fee is due\n";
        return;
    }

    const int toll = computeToll(game, visitor);
    game.payPlayer(visitor, *owner_, toll);
}
