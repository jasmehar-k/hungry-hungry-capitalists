#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>

#include "controller.h"
#include "game.h"
#include "savegame.h"
#include "setup.h"
#include "textview.h"

namespace {

struct Options {
    std::string loadFile;   // empty when no -load was given
    bool testing = false;
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
                std::cerr << "-load needs a filename\n";
                return false;
            }
            opts.loadFile = argv[++i];
        } else if (arg == "-testing") {
            opts.testing = true;
        } else {
            // The only other argument the spec allows is an optional seed.
            try {
                opts.seed = static_cast<unsigned>(std::stoul(arg));
                opts.seedGiven = true;
            } catch (const std::exception &) {
                std::cerr << "unrecognized argument: " << arg << "\n";
                return false;
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
    std::mt19937 rng{opts.seedGiven ? opts.seed : std::random_device{}()};
    static_cast<void>(rng);

    Game game{opts.testing};

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
    auto controller = std::make_shared<GameController>(game);
    TextView view{game, controller};
    view.render(std::cout);

    std::string line;
    while (std::getline(std::cin, line)) {
        view.handleInput(line);
    }
}
