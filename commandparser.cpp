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
    if (word == "insolvent") return std::make_unique<InsolventCommand>(game);
    if (word == "assets") return std::make_unique<AssetsCommand>(game);
    if (word == "all") return std::make_unique<AllCommand>(game);

    if (word == "accept") return std::make_unique<AcceptCommand>(game);
    if (word == "reject") return std::make_unique<RejectCommand>(game);

    if (word == "trade") {
        std::string target;
        std::string give;
        std::string receive;
        if (!(input >> target >> give >> receive)) return nullptr;
        return std::make_unique<TradeCommand>(game, target, give, receive);
    }

    if (word == "upgrade") {
        std::string district;
        std::string action;
        if (!(input >> district >> action)) return nullptr;
        if (action == "buy") return std::make_unique<UpgradeCommand>(game, district, true);
        if (action == "sell") return std::make_unique<UpgradeCommand>(game, district, false);
        return nullptr;
    }

    // The remaining commands each take one argument, and a missing argument is
    // as unrecognizable as a misspelled verb.
    std::string argument;
    if (!(input >> argument)) return nullptr;

    if (word == "mortgage") return std::make_unique<MortgageCommand>(game, argument);
    if (word == "unmortgage") return std::make_unique<UnmortgageCommand>(game, argument);
    if (word == "save") return std::make_unique<SaveCommand>(game, argument);

    return nullptr;
}
