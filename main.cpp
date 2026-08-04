#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>

#include "cards.h"
#include "controller.h"
#include "game.h"
#include "player.h"
#include "savegame.h"
#include "setup.h"
#include "textview.h"
#include "turn.h"

namespace {

struct Options {
    std::string loadFile;   // empty when no -load was given
    bool testing = false;
    bool shuffledDeck = false;
    unsigned seed = 0;
    bool seedGiven = false;
};

// The spec fixes -load as the first argument and -testing as the second, but
// accepting them in any order costs nothing and is friendlier at the demo.
bool parseArguments(int argc, char *argv[], Options &opts) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-load") {
            if (i + 1 >= argc) {
                std::cerr << "-load needs a filename; starting a new game instead\n";
                continue;
            }
            opts.loadFile = argv[++i];
        } else if (arg == "-testing") {
            opts.testing = true;
        } else if (arg == "-deck") {
            // Optional enhancement, selected without recompiling.
            opts.shuffledDeck = true;
        } else {
            // The only other argument the spec allows is an optional seed.
            try {
                opts.seed = static_cast<unsigned>(std::stoul(arg));
                opts.seedGiven = true;
            } catch (const std::exception &) {
                // Warn and carry on rather than refusing to start. A mistyped
                // flag at the demo should not stop the game from running.
                std::cerr << "ignoring unrecognized argument: " << arg << "\n";
            }
        }
    }
    return true;
}

}  // namespace

int main(int argc, char *argv[]) {
    Options opts;
    if (!parseArguments(argc, argv, opts)) return 1;

    // Seeded once at startup so that a given seed replays an identical game.
    Game game{opts.testing, opts.seedGiven ? opts.seed : std::random_device{}(),
              opts.shuffledDeck};
    if (opts.shuffledDeck) {
        std::cout << "Enchantment effects come from a shuffled deck of "
                  << CardDeck::size() << " cards\n";
    }

    if (!opts.loadFile.empty()) {
        std::string error;
        if (!SaveGame::load(game, opts.loadFile, error)) {
            std::cerr << "could not load " << opts.loadFile << ": " << error << "\n";
            return 1;
        }
    } else if (!Setup::runNewGame(game, std::cin, std::cout)) {
        std::cerr << "setup abandoned\n";
        return 1;
    }

    // The view attaches only once the roster is settled: it observes the Game,
    // and attaching earlier would redraw the board once per Artificer added.
    game.turn().beginTurn();

    auto controller = std::make_shared<GameController>(game);
    TextView view{game, controller};
    view.render(std::cout);

    std::string line;
    while (!game.isOver() && std::getline(std::cin, line)) {
        view.handleInput(line);
    }

    if (const Player *champion = game.winner()) {
        std::cout << champion->name() << " wins ArcaneMarket\n";
    }
}
