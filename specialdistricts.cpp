#include "specialdistricts.h"

#include <ostream>
#include <string>

#include "boarddata.h"
#include "cards.h"
#include "game.h"
#include "player.h"

// Passing or landing here pays the bonus, which Game::movePlayer handles for
// both cases, so there is nothing left to do on arrival.
void GrandEntrance::landOn(Game &, Player &) {}

// Landing on The Lockup does nothing; only being sent here detains anyone.
void Lockup::landOn(Game &game, Player &visitor) {
    game.out() << visitor.name() << " is just visiting The Lockup\n";
}

void FreeRest::landOn(Game &, Player &) {}

void GoToLockup::landOn(Game &game, Player &visitor) {
    // Sent directly, so no Grand Entrance bonus is collected on the way.
    game.sendToLockup(visitor);
}

void GuildLevy::landOn(Game &game, Player &visitor) {
    const int share = game.totalWorth(visitor) * kGuildLevyPercent / 100;
    game.out() << "Guild Levy: pay " << kGuildLevyFlat << " Reagents, or "
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
        game.out() << "please answer flat or percent\n";
    }
}

void TransitToll::landOn(Game &game, Player &visitor) {
    game.payTreasury(visitor, kTransitTollFee);
}

void MarketFestival::landOn(Game &game, Player &visitor) {
    game.out() << visitor.name() << "'s cart is jostled by the festival crowds\n";
}

// Markets sit at squares 5, 15, 28 and 36, more than three apart, so a move of
// one to three can never land on another Market; the two absolute outcomes are
// squares 0 and 10, which are not Markets either. No recursion guard is needed.
void WanderingMarket::landOn(Game &game, Player &visitor) {
    if (game.tryGrantFreePass(visitor)) return;

    const MarketOutcome &outcome = drawMarketOutcome(game.rng());
    game.out() << "a Wandering Market sends " << visitor.name() << ' ' << outcome.description
               << '\n';

    switch (outcome.move) {
        case MarketMove::Back3: game.movePlayer(visitor, -3); return;
        case MarketMove::Back2: game.movePlayer(visitor, -2); return;
        case MarketMove::Back1: game.movePlayer(visitor, -1); return;
        case MarketMove::Forward1: game.movePlayer(visitor, 1); return;
        case MarketMove::Forward2: game.movePlayer(visitor, 2); return;
        case MarketMove::Forward3: game.movePlayer(visitor, 3); return;
        case MarketMove::ToLockup: game.sendToLockup(visitor); return;
        case MarketMove::ToGrandEntrance:
            // Moving forward to square 0 collects the bonus by the normal path
            // rather than needing a special case here.
            game.movePlayer(visitor, (kBoardSize - visitor.position()) % kBoardSize);
            return;
    }
}

void EnchantmentDeck::landOn(Game &game, Player &visitor) {
    if (game.tryGrantFreePass(visitor)) return;

    // Either the spec's probability table or the shuffled deck, chosen at
    // startup; the square itself does not care which.
    const int change = game.drawEnchantmentValue();
    game.out() << visitor.name() << " draws an Enchantment card: " << (change > 0 ? "+" : "")
               << change << " Reagents\n";

    if (change < 0) {
        // Routed through payTreasury so a loss the Artificer cannot cover
        // becomes a debt rather than a negative balance.
        game.payTreasury(visitor, -change);
    } else {
        visitor.addReagents(change);
    }
}
