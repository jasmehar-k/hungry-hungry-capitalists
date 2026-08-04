#include "specialdistricts.h"

// Every body below is filled in during the movement step; the hierarchy exists
// now so the board can be built and drawn.

// Passing or landing here pays kGrandEntranceBonus, handled by the mover.
void GrandEntrance::landOn(Game &, Player &) {}

// Landing on The Lockup does nothing; only being sent here detains an Artificer.
void Lockup::landOn(Game &, Player &) {}

void FreeRest::landOn(Game &, Player &) {}

// TODO(movement step): send the visitor to The Lockup without the Grand
// Entrance bonus.
void GoToLockup::landOn(Game &, Player &) {}

// TODO(movement step): 300 Reagents or 10% of total worth, the visitor chooses.
void GuildLevy::landOn(Game &, Player &) {}

// TODO(movement step): 150 Reagents to the Guild Treasury.
void TransitToll::landOn(Game &, Player &) {}

void MarketFestival::landOn(Game &, Player &) {}

// TODO(cards step): redirect per Table 2, then resolve the destination.
void WanderingMarket::landOn(Game &, Player &) {}

// TODO(cards step): draw per Table 3, print the effect, then apply it.
void EnchantmentDeck::landOn(Game &, Player &) {}
