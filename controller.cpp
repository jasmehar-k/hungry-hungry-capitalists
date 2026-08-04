#include "controller.h"

#include <iostream>

#include "command.h"
#include "commandparser.h"
#include "game.h"

GameController::GameController(Game &game) : game_{game} {}

// The Invoker: it builds a Command for the line and fires it through the
// interface, knowing nothing about what any particular command does.
void GameController::handleCommand(const std::string &line) {
    if (line.find_first_not_of(" \t\r") == std::string::npos) return;

    std::unique_ptr<Command> command = CommandParser::parse(game_, line);
    if (!command) {
        std::cout << "I don't understand \"" << line << "\"\n";
    } else {
        command->execute();
    }

    // The spec redraws the board after every command, including ones that
    // changed nothing, so the notification happens here rather than inside
    // each Model mutator.
    game_.notifyObservers();
}
