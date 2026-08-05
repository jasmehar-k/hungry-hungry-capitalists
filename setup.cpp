#include "setup.h"

#include <istream>
#include <ostream>
#include <sstream>
#include <string>

#include "game.h"
#include "player.h"

// The new-game dialogue, kept out of Game because it runs exactly once and is
// not a rule of play. Its streams are parameters rather than std::cin/cout, so
// a scripted roster can be fed in the same way the tests feed commands.
//
// Every prompt loops until the answer is usable: the roster is what the whole
// game is built on, so an invalid one is refused here rather than left to fail
// somewhere later. Reaching end of input returns false and abandons setup,
// which is the one case that is not the user's mistake.

namespace {

// Only the tokens still free are offered, so the display and the validation
// below cannot disagree about what is choosable.
void listAvailableTokens(const Game &game, std::ostream &out) {
    out << "available tokens:";
    for (const TokenChoice &choice : kTokens) {
        if (!game.tokenTaken(choice.symbol)) {
            out << ' ' << choice.symbol << " (" << choice.name << ')';
        }
    }
    out << '\n';
}

// Names identify Artificers in the save file, so two Artificers sharing one
// would make a saved game ambiguous about who owns what.
bool nameTaken(const Game &game, const std::string &name) {
    for (std::size_t i = 0; i < game.playerCount(); ++i) {
        if (game.player(i).name() == name) return true;
    }
    return false;
}

}  // namespace

bool Setup::runNewGame(Game &game, std::istream &in, std::ostream &out) {
    std::string line;

    std::size_t count = 0;
    while (true) {
        out << "how many Artificers? (2-6): ";
        if (!std::getline(in, line)) return false;
        std::istringstream input{line};
        if (input >> count && count >= kMinPlayers && count <= kMaxPlayers) break;
        out << "please enter a number from 2 to 6\n";
    }

    for (std::size_t i = 0; i < count; ++i) {
        std::string name;
        while (true) {
            out << "name for Artificer " << (i + 1) << ": ";
            if (!std::getline(in, line)) return false;
            std::istringstream input{line};
            // A single word keeps the save file's whitespace-separated format
            // unambiguous when the game is written back out.
            if (!(input >> name)) {
                out << "please enter a name\n";
                continue;
            }
            // GUILD marks an unowned district in the save file, so an
            // Artificer of that name would be indistinguishable from one.
            if (name == "GUILD") {
                out << "GUILD is reserved for unowned districts\n";
                continue;
            }
            if (nameTaken(game, name)) {
                out << "that name is already taken\n";
                continue;
            }
            break;
        }

        char token = ' ';
        while (true) {
            listAvailableTokens(game, out);
            out << "token for " << name << ": ";
            if (!std::getline(in, line)) return false;
            std::istringstream input{line};
            if (input >> token && isValidToken(token) && !game.tokenTaken(token)) break;
            out << "please choose one of the available tokens\n";
        }

        game.addPlayer(name, token);
    }
    return true;
}
