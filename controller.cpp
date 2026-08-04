#include "controller.h"

#include <iostream>
#include <sstream>

#include "game.h"

GameController::GameController(Game &game) : game_{game} {}

void GameController::handleCommand(const std::string &line) {
    std::istringstream input{line};
    std::string command;
    if (!(input >> command)) return;  // a blank line is not an error

    // The full interpreter arrives with the command step; until then every
    // command is unrecognized, which is also the graceful-failure path a
    // misspelling must take.
    if (command == "next") {
        game_.endTurn();
        return;
    }

    std::cout << "I don't know the command \"" << command << "\"\n";
}
