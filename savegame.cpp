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

namespace {

constexpr int kMortgagedMarker = -1;
constexpr int kMaxFreePassesInPlay = 4;

// The file separates its sections with blank lines, so every read skips them.
bool nextLine(std::istream &in, std::string &line) {
    while (std::getline(in, line)) {
        if (line.find_first_not_of(" \t\r") != std::string::npos) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return true;
        }
    }
    return false;
}

// One record per Artificer, held until the whole file validates.
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

std::vector<const OwnableDistrict *> ownablesInOrder(const Board &board) {
    std::vector<const OwnableDistrict *> result;
    for (int square = 0; square < Board::size(); ++square) {
        if (const auto *ownable = dynamic_cast<const OwnableDistrict *>(&board.at(square))) {
            result.push_back(ownable);
        }
    }
    return result;
}

bool parsePlayer(const std::string &line, PlayerRecord &record, std::string &error) {
    std::istringstream input{line};
    if (!(input >> record.name >> record.token >> record.freePasses >> record.reagents >>
          record.position)) {
        error = "malformed Artificer line: " + line;
        return false;
    }
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

bool validatePlayer(const PlayerRecord &record, std::string &error) {
    if (!isValidToken(record.token)) {
        error = "unknown token '" + std::string(1, record.token) + "' for " + record.name;
        return false;
    }
    if (record.position < 0 || record.position >= kBoardSize) {
        error = record.name + " is on square " + std::to_string(record.position);
        return false;
    }
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
    if (totalFreePasses > kMaxFreePassesInPlay) {
        error = "there are " + std::to_string(totalFreePasses) +
                " Free-Pass Tokens in play, which exceeds the limit of 4";
        return false;
    }

    const std::vector<const OwnableDistrict *> expected = ownablesInOrder(game.board());
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
        const bool isWorkshop = ownable->kind() == DistrictKind::Workshop;
        const int ceiling = isWorkshop ? kMaxUpgrades : 0;
        if (record.upgrades < kMortgagedMarker || record.upgrades > ceiling) {
            error = record.name + " has an upgrade value of " + std::to_string(record.upgrades) +
                    ", which is outside -1 to " + std::to_string(ceiling);
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
        if (auto *workshop = dynamic_cast<Workshop *>(ownable)) {
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

    out << game.playerCount() << "\n\n";
    for (std::size_t i = 0; i < game.playerCount(); ++i) {
        const Player &player = game.player(i);
        out << player.name() << ' ' << player.token() << ' ' << player.freePasses() << ' '
            << player.reagents() << ' ' << player.position();
        if (player.position() == kLockupSquare) {
            out << ' ' << (player.inLockup() ? 1 : 0);
            if (player.inLockup()) out << ' ' << player.lockupTurns();
        }
        out << '\n';
    }

    out << '\n';
    for (const OwnableDistrict *ownable : ownablesInOrder(game.board())) {
        out << ownable->name() << ' '
            << (ownable->owner() ? ownable->owner()->name() : "GUILD") << ' '
            << (ownable->isMortgaged() ? kMortgagedMarker : ownable->upgradeCount()) << '\n';
    }
    return static_cast<bool>(out);
}
