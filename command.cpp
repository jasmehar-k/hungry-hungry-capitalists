#include "command.h"

#include <iostream>

#include "game.h"
#include "player.h"
#include "savegame.h"
#include "turn.h"

void RollCommand::execute() { game_.turn().roll(die1_, die2_); }

void NextCommand::execute() { game_.turn().next(); }

void MortgageCommand::execute() { game_.mortgage(district_); }

void UnmortgageCommand::execute() { game_.unmortgage(district_); }

void InsolventCommand::execute() { game_.declareInsolvency(); }

void AssetsCommand::execute() {
    // Unavailable while a Guild Levy is being resolved; that prompt reads its
    // own input, so no command can reach here mid-levy.
    game_.printAssets(game_.currentPlayer());
}

void AllCommand::execute() { game_.printAllAssets(); }

void SaveCommand::execute() {
    if (SaveGame::save(game_, filename_)) {
        std::cout << "saved to " << filename_ << '\n';
    } else {
        std::cout << "could not write " << filename_ << '\n';
    }
}
