#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

#include <memory>
#include <string>

class Command;
class Game;

// Turns one line of input into a Command, or nullptr when nothing matches.
// That single nullptr path is what makes misspelled commands fail gracefully.
//
// Like DistrictFactory this is the Simple Factory idiom, not GoF Factory
// Method: the set of commands is fixed by the spec and there is no creator
// hierarchy.
class CommandParser {
public:
    static std::unique_ptr<Command> parse(Game &game, const std::string &line);
};

#endif  // COMMANDPARSER_H
