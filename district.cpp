#include "district.h"

#include <ostream>
#include <utility>

#include "game.h"
#include "player.h"

District::District(int square, std::string name)
    : square_{square}, name_{std::move(name)} {}

OwnableDistrict::OwnableDistrict(int square, std::string name, int buyPrice)
    : District{square, std::move(name)}, buyPrice_{buyPrice} {}

// The board cell shows the owner's token; a blank means the Guild still holds
// the district. Returning a char rather than the Player keeps the view from
// having to reason about a null owner.
char OwnableDistrict::ownerInitial() const {
    return owner_ ? owner_->token() : ' ';
}

// Template Method: this is the invariant skeleton of landing on any ownable
// district, and it is `final` so no subclass can reorder or skip a step. Only
// computeToll, the one step that genuinely differs, is left to the subclass.
//
// The three early exits are shared rules that would otherwise be repeated in
// all three subclasses: an unowned district is offered for sale, an Artificer
// never tolls themselves, and a mortgaged district earns nothing.
void OwnableDistrict::landOn(Game &game, Player &visitor) {
    if (owner_ == nullptr) {
        game.offerPurchase(*this, visitor);
        return;
    }
    if (owner_ == &visitor) return;  // an Artificer never tolls themselves
    if (mortgaged_) {
        game.out() << name() << " is mortgaged, so no Toll Fee is due\n";
        return;
    }

    // The hook. Through payPlayer so that a toll larger than the visitor holds
    // becomes a parked debt rather than a negative balance.
    const int toll = computeToll(game, visitor);
    game.payPlayer(visitor, *owner_, toll);
}
