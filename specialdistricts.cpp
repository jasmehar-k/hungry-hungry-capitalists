#include "specialdistricts.h"

#include <iostream>
#include <string>

#include "boarddata.h"
#include "game.h"
#include "player.h"

// Passing or landing here pays the bonus, which Game::movePlayer handles for
// both cases, so there is nothing left to do on arrival.
void GrandEntrance::landOn(Game &, Player &) {}

// Landing on The Lockup does nothing; only being sent here detains anyone.
void Lockup::landOn(Game &, Player &visitor) {
    std::cout << visitor.name() << " is just visiting The Lockup\n";
}

void FreeRest::landOn(Game &, Player &) {}

void GoToLockup::landOn(Game &game, Player &visitor) {
    // Sent directly, so no Grand Entrance bonus is collected on the way.
    game.sendToLockup(visitor);
}

void GuildLevy::landOn(Game &game, Player &visitor) {
    const int share = game.totalWorth(visitor) * kGuildLevyPercent / 100;
    std::cout << "Guild Levy: pay " << kGuildLevyFlat << " Reagents, or "
              << kGuildLevyPercent << "% of your worth which is " << share << " Reagents\n";

    while (true) {
        const std::string answer = game.prompt("flat or percent? ");
        if (answer == "percent") {
            game.payTreasury(visitor, share);
            return;
        }
        // A blank answer (including end of input) takes the flat rate so the
        // prompt can never spin forever on a closed stream.
        if (answer == "flat" || answer.empty()) {
            game.payTreasury(visitor, kGuildLevyFlat);
            return;
        }
        std::cout << "please answer flat or percent\n";
    }
}

void TransitToll::landOn(Game &game, Player &visitor) {
    game.payTreasury(visitor, kTransitTollFee);
}

void MarketFestival::landOn(Game &, Player &visitor) {
    std::cout << visitor.name() << "'s cart is jostled by the festival crowds\n";
}

// TODO(cards step): redirect per Table 2, then resolve the destination.
void WanderingMarket::landOn(Game &, Player &) {}

// TODO(cards step): draw per Table 3, print the effect, then apply it.
void EnchantmentDeck::landOn(Game &, Player &) {}
