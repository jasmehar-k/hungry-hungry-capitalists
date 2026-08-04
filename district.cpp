#include "district.h"

#include <utility>

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
    if (owner_ == nullptr) return;   // unowned: purchase or auction
    if (owner_ == &visitor) return;  // an Artificer never tolls themselves
    if (mortgaged_) return;          // a mortgaged district charges nothing

    const int toll = computeToll(game, visitor);
    static_cast<void>(toll);  // charging arrives with the movement step
}
