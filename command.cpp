#include "command.h"

#include <cstddef>
#include <ostream>

#include "district.h"
#include "game.h"
#include "player.h"
#include "savegame.h"

void MessageCommand::execute() { out_ << message_ << '\n'; }

void RollCommand::execute() { game_.roll(die1_, die2_); }

void NextCommand::execute() { game_.next(); }

void MortgageCommand::execute() { game_.mortgage(district_); }

void UnmortgageCommand::execute() { game_.unmortgage(district_); }

void TradeCommand::execute() { game_.offerTrade(target_, give_, receive_); }

void AcceptCommand::execute() { game_.acceptTrade(); }

void RejectCommand::execute() { game_.rejectTrade(); }

void UpgradeCommand::execute() {
    if (buying_) {
        game_.buyUpgrade(district_);
    } else {
        game_.sellUpgrade(district_);
    }
}

void InsolventCommand::execute() { game_.declareInsolvency(); }

namespace {

// Formatting lives with the command rather than the Model: assets are one
// presentation of the Model's data, and the Model only supplies the data.
void printAssets(Game &game, const Player &player) {
    game.out() << player.name() << " (" << player.token() << ") holds " << player.reagents()
               << " Reagents, " << player.freePasses() << " Free-Pass Tokens\n";
    for (const OwnableDistrict *ownable : game.districtsOwnedBy(player)) {
        game.out() << "  " << ownable->name();
        if (ownable->isMortgaged()) {
            game.out() << " (mortgaged)";
        } else if (ownable->upgradeCount() > 0) {
            game.out() << " (" << ownable->upgradeCount() << " upgrades)";
        }
        game.out() << '\n';
    }
}

}  // namespace

void AssetsCommand::execute() {
    // Unavailable while a Guild Levy is being resolved; that prompt reads its
    // own input, so no command can reach here mid-levy.
    printAssets(game_, game_.currentPlayer());
}

void AllCommand::execute() {
    for (std::size_t i = 0; i < game_.playerCount(); ++i) {
        const Player &player = game_.player(i);
        if (player.isInsolvent()) {
            game_.out() << player.name() << " is insolvent\n";
            continue;
        }
        printAssets(game_, player);
    }
}

void SaveCommand::execute() {
    if (SaveGame::save(game_, filename_)) {
        game_.out() << "saved to " << filename_ << '\n';
    } else {
        game_.out() << "could not write " << filename_ << '\n';
    }
}
