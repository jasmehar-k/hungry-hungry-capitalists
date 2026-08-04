#include "command.h"

#include <ostream>

#include "game.h"
#include "player.h"
#include "savegame.h"
#include "turn.h"

void MessageCommand::execute() { out_ << message_ << '\n'; }

void RollCommand::execute() { game_.turn().roll(die1_, die2_); }

void NextCommand::execute() { game_.turn().next(); }

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

void AssetsCommand::execute() {
    // Unavailable while a Guild Levy is being resolved; that prompt reads its
    // own input, so no command can reach here mid-levy.
    game_.printAssets(game_.currentPlayer());
}

void AllCommand::execute() { game_.printAllAssets(); }

void SaveCommand::execute() {
    if (SaveGame::save(game_, filename_)) {
        game_.out() << "saved to " << filename_ << '\n';
    } else {
        game_.out() << "could not write " << filename_ << '\n';
    }
}
