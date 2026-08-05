#include "savegame.h"

#include <fstream>
#include <sstream>
#include <vector>

#include "board.h"
#include "boarddata.h"
#include "district.h"
#include "game.h"
#include "player.h"
#include "workshop.h"

// The load is deliberately in two phases: everything is parsed into records and
// checked first, and the game is only mutated once the whole file has passed.
// A loader that mutated as it read would leave a half-restored game behind on
// any error, which is worse than not loading at all — the spec warns that a
// game which fails to load costs significant marks, and the demo has no way
// back from a corrupted board.
//
// Save and load live in one file so the two halves of the format cannot drift
// apart: any change to what is written has to be made next to what reads it.

namespace {

// Upgrades and mortgage share one field. A count of 0 to 5 is a real upgrade
// level; -1 means mortgaged, which is why a mortgaged district always restores
// with no upgrades under it.
constexpr int kMortgagedMarker = -1;

// The file separates its sections with blank lines, so every read skips them.
// Trailing carriage returns are dropped as well, so a file written on Windows
// and loaded on the student server does not fail on invisible characters.
bool nextLine(std::istream &in, std::string &line) {
    while (std::getline(in, line)) {
        if (line.find_first_not_of(" \t\r") != std::string::npos) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return true;
        }
    }
    return false;
}

// One record per Artificer, held until the whole file validates. These exist
// so that parsing can finish before anything is committed to the live game.
struct PlayerRecord {
    std::string name;
    char token = ' ';
    int freePasses = 0;
    int reagents = 0;
    int position = 0;
    bool detained = false;
    int lockupTurns = 0;
};

struct DistrictRecord {
    std::string name;
    std::string owner;
    int upgrades = 0;
};

bool parsePlayer(const std::string &line, PlayerRecord &record, std::string &error) {
    std::istringstream input{line};
    if (!(input >> record.name >> record.token >> record.freePasses >> record.reagents >>
          record.position)) {
        error = "malformed Artificer line: " + line;
        return false;
    }
    // Square 10 is the only square that carries extra fields, because it is
    // the only one where two different situations look the same: an Artificer
    // merely visiting The Lockup and one detained in it.
    if (record.position == kLockupSquare) {
        int detainedFlag = 0;
        if (input >> detainedFlag && detainedFlag == 1) {
            if (!(input >> record.lockupTurns)) {
                error = "a detained Artificer needs a turn count: " + line;
                return false;
            }
            record.detained = true;
        }
    }
    return true;
}

// Each of these refuses a file that would produce a game the rules cannot
// describe. Every message names the Artificer and the value, because the only
// thing the user can do with a rejected save is go and look at the file.
bool validatePlayer(const PlayerRecord &record, std::string &error) {
    if (!isValidToken(record.token)) {
        error = "unknown token '" + std::string(1, record.token) + "' for " + record.name;
        return false;
    }
    if (record.position < 0 || record.position >= kBoardSize) {
        error = record.name + " is on square " + std::to_string(record.position);
        return false;
    }
    // Square 30 sends whoever lands on it to The Lockup, so nobody can
    // legitimately be standing on it when a game is saved.
    if (record.position == kGoToLockupSquare) {
        error = record.name + " cannot start on square 30 (Go to Lockup)";
        return false;
    }
    if (record.freePasses < 0) {
        error = record.name + " has a negative Free-Pass count";
        return false;
    }
    if (record.detained &&
        (record.lockupTurns < 0 || record.lockupTurns >= kMaxLockupTurns)) {
        error = record.name + " has served " + std::to_string(record.lockupTurns) +
                " Lockup turns, which is outside 0-2";
        return false;
    }
    return true;
}

}  // namespace

bool SaveGame::load(Game &game, const std::string &filename, std::string &error) {
    std::ifstream in{filename};
    if (!in) {
        error = "cannot open " + filename;
        return false;
    }

    std::string line;
    if (!nextLine(in, line)) {
        error = filename + " is empty";
        return false;
    }

    std::size_t numPlayers = 0;
    {
        std::istringstream input{line};
        if (!(input >> numPlayers)) {
            error = "the first line must be the number of Artificers";
            return false;
        }
    }
    if (numPlayers < kMinPlayers || numPlayers > kMaxPlayers) {
        error = "ArcaneMarket takes 2 to 6 Artificers, not " + std::to_string(numPlayers);
        return false;
    }

    std::vector<PlayerRecord> players;
    int totalFreePasses = 0;
    for (std::size_t i = 0; i < numPlayers; ++i) {
        if (!nextLine(in, line)) {
            error = "the file ends before all Artificers are listed";
            return false;
        }
        PlayerRecord record;
        if (!parsePlayer(line, record, error)) return false;
        if (!validatePlayer(record, error)) return false;
        for (const PlayerRecord &earlier : players) {
            if (earlier.token == record.token) {
                error = "two Artificers share the token '" + std::string(1, record.token) + "'";
                return false;
            }
            if (earlier.name == record.name) {
                error = "two Artificers share the name " + record.name;
                return false;
            }
        }
        totalFreePasses += record.freePasses;
        players.push_back(record);
    }
    // The cap is global rather than per Artificer, so it can only be checked
    // once every Artificer has been read.
    if (totalFreePasses > kMaxFreePassesInPlay) {
        error = "there are " + std::to_string(totalFreePasses) +
                " Free-Pass Tokens in play, which exceeds the limit of 4";
        return false;
    }

    // The board's registry is already in circuit order, which is the order the
    // save format requires.
    const std::vector<OwnableDistrict *> &expected = game.board().ownables();
    std::vector<DistrictRecord> districts;
    for (const OwnableDistrict *ownable : expected) {
        if (!nextLine(in, line)) {
            error = "the file ends before all districts are listed";
            return false;
        }
        DistrictRecord record;
        std::istringstream input{line};
        if (!(input >> record.name >> record.owner >> record.upgrades)) {
            error = "malformed district line: " + line;
            return false;
        }
        // The format identifies districts by position, not by name alone, so a
        // file listing them in another order is rejected rather than
        // rearranged: silently accepting it would hide a corrupted file.
        if (record.name != ownable->name()) {
            error = "districts must appear in circuit order: expected " + ownable->name() +
                    ", found " + record.name;
            return false;
        }
        if (record.owner != "GUILD") {
            bool known = false;
            for (const PlayerRecord &player : players) {
                if (player.name == record.owner) known = true;
            }
            if (!known) {
                error = record.name + " is owned by unknown Artificer " + record.owner;
                return false;
            }
        }
        // Only Workshops take upgrades, so the ceiling depends on what kind of
        // district this position holds. A Forge or Depot may still be
        // mortgaged, which is why the floor is the same for all of them.
        const bool isWorkshop = ownable->kind() == DistrictKind::Workshop;
        const int ceiling = isWorkshop ? kMaxUpgrades : 0;
        if (record.upgrades < kMortgagedMarker || record.upgrades > ceiling) {
            error = record.name + " has an upgrade value of " + std::to_string(record.upgrades) +
                    ", which is outside -1 to " + std::to_string(ceiling);
            return false;
        }
        // Owner and upgrades are not independent. Mortgaging and upgrading are
        // both owner actions, so a district held by the Guild must be plain.
        if (record.owner == "GUILD" && record.upgrades != 0) {
            error = record.name + " is unowned, so it cannot be " +
                    (record.upgrades == kMortgagedMarker ? "mortgaged" : "upgraded");
            return false;
        }
        districts.push_back(record);
    }

    // Everything validated; only now is the game mutated.
    for (const PlayerRecord &record : players) {
        game.addPlayer(record.name, record.token);
        Player &player = game.player(game.playerCount() - 1);
        player.setReagents(record.reagents);
        player.setPosition(record.position);
        player.setFreePasses(record.freePasses);
        player.setLockupState(record.detained, record.lockupTurns);
    }

    // Names were already checked against the circuit above, so every lookup
    // here is known to succeed.
    for (const DistrictRecord &record : districts) {
        OwnableDistrict *ownable = game.board().findOwnable(record.name);
        if (record.owner == "GUILD") {
            ownable->setOwner(nullptr);
        } else {
            for (std::size_t i = 0; i < game.playerCount(); ++i) {
                if (game.player(i).name() == record.owner) ownable->setOwner(&game.player(i));
            }
        }
        ownable->setMortgaged(record.upgrades == kMortgagedMarker);
        if (Workshop *workshop = game.board().findWorkshop(record.name)) {
            workshop->setUpgrades(record.upgrades == kMortgagedMarker ? 0 : record.upgrades);
        }
    }

    // "Play resumes with the first-listed Artificer."
    game.setCurrentPlayer(0);
    return true;
}

bool SaveGame::save(const Game &game, const std::string &filename) {
    std::ofstream out{filename};
    if (!out) return false;

    // Only solvent Artificers are written. The format has no field for
    // insolvency, so writing an eliminated Artificer would resurrect them as a
    // normal player with 0 Reagents the next time the file is loaded.
    out << game.solventCount() << "\n\n";
    for (std::size_t i = 0; i < game.playerCount(); ++i) {
        const Player &player = game.player(i);
        if (player.isInsolvent()) continue;
        out << player.name() << ' ' << player.token() << ' ' << player.freePasses() << ' '
            << player.reagents() << ' ' << player.position();
        if (player.position() == kLockupSquare) {
            out << ' ' << (player.inLockup() ? 1 : 0);
            if (player.inLockup()) out << ' ' << player.lockupTurns();
        }
        out << '\n';
    }

    out << '\n';
    for (const OwnableDistrict *ownable : game.board().ownables()) {
        out << ownable->name() << ' '
            << (ownable->owner() ? ownable->owner()->name() : "GUILD") << ' '
            << (ownable->isMortgaged() ? kMortgagedMarker : ownable->upgradeCount()) << '\n';
    }
    return static_cast<bool>(out);
}
