#include "commandparser.h"

#include <sstream>

#include "command.h"
#include "game.h"

// Reading a line and carrying it out are two different jobs, and this file
// only does the first: it decides which Command object the line describes and
// builds it with its arguments already parsed. Returning nullptr for anything
// it cannot place is what makes a misspelled command harmless, since the
// Controller has exactly one path for "no command matched".
//
// The parser prints nothing itself. Where a line deserves a specific
// explanation rather than the generic one, it returns a MessageCommand
// carrying that text, which keeps the diagnostic with the command layer
// instead of putting console output in the parser.

namespace {

// A die value must be a whole non-negative number and nothing else, so "4x"
// and "abc" are both refused rather than half-read.
bool parseDie(const std::string &text, int &out) {
    std::istringstream parsed{text};
    int value = 0;
    if (!(parsed >> value) || value < 0) return false;
    char trailing = 0;
    if (parsed >> trailing) return false;
    out = value;
    return true;
}

}  // namespace

std::unique_ptr<Command> CommandParser::parse(Game &game, const std::string &line) {
    std::istringstream input{line};
    std::string word;
    if (!(input >> word)) return nullptr;

    if (word == "roll") {
        std::string first;
        if (!(input >> first)) return std::make_unique<RollCommand>(game);

        // Anything after "roll" is an attempt at explicit dice. Silently
        // rolling at random instead would hide the user's mistake.
        std::string second;
        int die1 = 0;
        int die2 = 0;
        if (!(input >> second) || !parseDie(first, die1) || !parseDie(second, die2)) {
            return std::make_unique<MessageCommand>(
                game.out(), "roll takes either no arguments or two non-negative dice values");
        }
        if (!game.testingMode()) {
            return std::make_unique<MessageCommand>(
                game.out(), "explicit dice values need -testing; plain \"roll\" rolls normally");
        }
        return std::make_unique<RollCommand>(game, die1, die2);
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
