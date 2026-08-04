// Headless checks of the Model, built and run with `make test`.
//
// The whole point of the MVC split and the injected streams is that the rules
// can be driven with no view attached: this harness constructs a Game over
// string streams, plays out movement, tolls, mortgages, levies, the Lockup,
// debts and insolvency, and asserts on the Model's state directly. It also
// proves the save format round-trips byte for byte, which is the spec's top
// marking priority.

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "board.h"
#include "boarddata.h"
#include "district.h"
#include "game.h"
#include "player.h"
#include "savegame.h"
#include "workshop.h"

namespace {

int checks = 0;
int failures = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        ++checks;                                                      \
        if (!(cond)) {                                                 \
            std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; \
            ++failures;                                                \
        }                                                              \
    } while (0)

std::string readFile(const std::string &name) {
    std::ifstream in{name};
    std::ostringstream all;
    all << in.rdbuf();
    return all.str();
}

void testBoardGeometry() {
    CHECK(Board::advance(39, 1) == 0);
    CHECK(Board::advance(2, -3) == 39);
    CHECK(Board::advance(0, 41) == 1);
    CHECK(Board::grandEntrancePasses(5, -3) == 0);  // backward never collects
    CHECK(Board::grandEntrancePasses(38, 4) == 1);
    CHECK(Board::grandEntrancePasses(0, 80) == 2);  // -testing can lap twice
}

void testRosterLimits() {
    std::istringstream in;
    std::ostringstream out;
    Game game{true, 1, false, in, out};
    CHECK(game.addPlayer("p1", 'G') && game.addPlayer("p2", 'O') &&
          game.addPlayer("p3", 'X') && game.addPlayer("p4", 'W') &&
          game.addPlayer("p5", 'A') && game.addPlayer("p6", 'P'));
    CHECK(!game.addPlayer("p7", 'G'));  // token already taken
    CHECK(!game.addPlayer("p7", 'O'));  // and a seventh Artificer regardless
}

void testGameRules() {
    std::istringstream in{"scripted-answer\n"};
    std::ostringstream out;
    Game game{true, 7, false, in, out};
    game.addPlayer("alice", 'G');
    game.addPlayer("bob", 'O');
    Player &alice = game.player(0);
    Player &bob = game.player(1);
    Board &board = game.board();

    // The prompt seam reads the injected stream, not the console.
    CHECK(game.prompt("q? ") == "scripted-answer");
    CHECK(out.str().find("q? ") != std::string::npos);

    // alice owns the whole Ember Guild; sizes are per guild, never assumed 3.
    for (const char *name : {"Tinder", "Flint", "Cinder-stone", "Slagpit"}) {
        board.findOwnable(name)->setOwner(&alice);
    }
    CHECK(game.guildComplete(Guild::Ember, alice));
    CHECK(!game.guildComplete(Guild::Smoke, alice));

    // Unimproved Workshop in a complete guild: the printed toll of 2 doubles.
    game.movePlayer(bob, 1);
    CHECK(bob.position() == 1);
    CHECK(bob.reagents() == 1496);
    CHECK(alice.reagents() == 1504);

    // Upgraded: the table value alone, plus one Grand Entrance bonus for the lap.
    Workshop *tinder = board.findWorkshop("Tinder");
    tinder->setUpgrades(3);
    game.movePlayer(bob, 40);
    CHECK(bob.reagents() == 1606);  // 1496 + 200 - 90
    CHECK(alice.reagents() == 1594);
    tinder->setUpgrades(0);

    // Mortgaging pays half; a mortgaged district tolls nothing; unmortgaging
    // costs 60% and goes to the Treasury.
    CHECK(game.mortgage("Tinder"));
    CHECK(tinder->isMortgaged());
    CHECK(alice.reagents() == 1624);
    game.movePlayer(bob, 40);
    CHECK(bob.reagents() == 1806);  // +200 lap, no toll due
    CHECK(!game.mortgage("Eastgate"));  // not alice's to mortgage
    CHECK(game.unmortgage("Tinder"));
    CHECK(alice.reagents() == 1588);
    CHECK(game.treasury() == 36);

    // Forge levy prices by how many the owner controls.
    board.findOwnable("Ironhold")->setOwner(&alice);
    board.findOwnable("Bronze-works")->setOwner(&alice);
    CHECK(game.countOwned(DistrictKind::Forge, alice) == 2);
    game.movePlayer(bob, 3);  // lands Ironhold
    CHECK(bob.reagents() == 1756);  // levy of 50 at two Forges
    CHECK(alice.reagents() == 1638);

    // Both Depots in one hand: ten times the dice total.
    board.findOwnable("Eastgate")->setOwner(&alice);
    board.findOwnable("Westgate")->setOwner(&alice);
    game.dice().set(3, 4);
    game.movePlayer(bob, 5);  // lands Eastgate
    CHECK(bob.reagents() == 1686);  // 10 x 7
    CHECK(alice.reagents() == 1708);

    // Total worth: savings + printed values + upgrades bought.
    CHECK(game.totalWorth(alice) == 1708 + 340 + 400 + 300);
    board.findWorkshop("Flint")->setUpgrades(2);
    CHECK(game.totalWorth(alice) == 1708 + 340 + 400 + 300 + 100);
    board.findWorkshop("Flint")->setUpgrades(0);
    CHECK(game.districtsOwnedBy(alice).size() == 8);

    // Four Free-Pass Tokens in play is a hard cap: the draw shuts off.
    alice.setFreePasses(2);
    bob.setFreePasses(2);
    CHECK(game.totalFreePasses() == 4);
    CHECK(!game.tryGrantFreePass(bob));
    alice.setFreePasses(1);
    bob.setFreePasses(0);

    // Detention state, as the save format must carry it.
    game.sendToLockup(bob);
    bob.serveLockupTurn();
    CHECK(bob.position() == kLockupSquare && bob.inLockup() && bob.lockupTurns() == 1);

    // Save, load into a fresh Game, save again: files must match byte for byte.
    const std::string file1 = "gametest_roundtrip1.txt";
    const std::string file2 = "gametest_roundtrip2.txt";
    CHECK(SaveGame::save(game, file1));

    std::istringstream in2;
    std::ostringstream out2;
    Game loaded{true, 7, false, in2, out2};
    std::string error;
    CHECK(SaveGame::load(loaded, file1, error));
    CHECK(error.empty());
    CHECK(loaded.playerCount() == 2);
    CHECK(loaded.player(0).name() == "alice" && loaded.player(0).reagents() == 1708);
    CHECK(loaded.player(1).inLockup() && loaded.player(1).lockupTurns() == 1);
    CHECK(loaded.player(0).freePasses() == 1);
    const OwnableDistrict *loadedTinder = loaded.board().findOwnable("Tinder");
    CHECK(loadedTinder->owner() == &loaded.player(0));
    CHECK(!loadedTinder->isMortgaged());
    CHECK(loaded.board().findOwnable("Ash-veil")->owner() == nullptr);
    CHECK(SaveGame::save(loaded, file2));
    CHECK(readFile(file1) == readFile(file2));
    std::remove(file1.c_str());
    std::remove(file2.c_str());

    // A payment beyond the debtor's holdings parks a debt instead of driving
    // the balance negative, and insolvency then ends the game at two players.
    game.setCurrentPlayer(1);
    bob.setReagents(10);
    game.payTreasury(bob, 500);
    CHECK(game.owes(bob));
    CHECK(game.amountOwed(bob) == 500);
    CHECK(bob.reagents() == 10);
    game.declareInsolvency();
    CHECK(bob.isInsolvent());
    CHECK(game.solventCount() == 1);
    CHECK(game.winner() == &alice);
    CHECK(game.isOver());
}

}  // namespace

int main() {
    testBoardGeometry();
    testRosterLimits();
    testGameRules();

    if (failures == 0) {
        std::cout << "gametest: all " << checks << " checks passed\n";
        return 0;
    }
    std::cout << "gametest: " << failures << " of " << checks << " checks failed\n";
    return 1;
}
