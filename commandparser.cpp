#include "commandparser.h"

#include <sstream>

#include "command.h"
#include "game.h"

std::unique_ptr<Command> CommandParser::parse(Game &game, const std::string &line) {
    std::istringstream input{line};
    std::string word;
    if (!(input >> word)) return nullptr;

    if (word == "roll") {
        int die1 = 0;
        int die2 = 0;
        // Explicit dice are only honoured under -testing; the plain roll still
        // works there, so a trailing pair is optional rather than required.
        if (game.testingMode() && (input >> die1 >> die2)) {
            if (die1 < 0 || die2 < 0) return nullptr;
            return std::make_unique<RollCommand>(game, die1, die2);
        }
        return std::make_unique<RollCommand>(game);
    }

    if (word == "next") return std::make_unique<NextCommand>(game);

    return nullptr;
}
