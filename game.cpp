#include "game.h"

#include <cassert>
#include <iostream>
#include <utility>

#include "boarddata.h"
#include "district.h"
#include "player.h"
#include "turn.h"
#include "workshop.h"

Game::Game(bool testingMode, unsigned seed)
    : testingMode_{testingMode}, rng_{seed}, dice_{rng_} {
    turn_ = std::make_unique<Turn>(*this);
}

Game::~Game() = default;

bool Game::tokenTaken(char token) const {
    for (const auto &player : players_) {
        if (player->token() == token) return true;
    }
    return false;
}

bool Game::addPlayer(std::string name, char token) {
    if (players_.size() >= kMaxPlayers || tokenTaken(token)) return false;
    players_.push_back(std::make_unique<Player>(std::move(name), token));
    return true;
}

const Player &Game::player(std::size_t index) const {
    assert(index < players_.size());
    return *players_[index];
}

Player &Game::player(std::size_t index) {
    assert(index < players_.size());
    return *players_[index];
}

const Player &Game::currentPlayer() const { return player(current_); }

Player &Game::currentPlayer() { return player(current_); }

void Game::setCurrentPlayer(std::size_t index) {
    assert(index < players_.size());
    current_ = index;
}

void Game::endTurn() {
    if (players_.empty()) return;
    current_ = (current_ + 1) % players_.size();
    turn_->beginTurn();
    std::cout << "it is now " << currentPlayer().name() << "'s turn\n";
}

void Game::movePlayer(Player &player, int steps) {
    const int from = player.position();
    const int to = Board::advance(from, steps);
    player.setPosition(to);

    if (Board::passedGrandEntrance(from, steps)) {
        player.addReagents(kGrandEntranceBonus);
        std::cout << player.name() << " passes the Grand Entrance and collects "
                  << kGrandEntranceBonus << " Reagents\n";
    }

    std::cout << player.name() << " lands on " << board_.at(to).name() << '\n';
    board_.at(to).landOn(*this, player);
}

void Game::sendToLockup(Player &player) {
    player.sendToLockup();
    std::cout << player.name() << " is sent to The Lockup\n";
}

bool Game::forceLockupExit(Player &player) {
    if (player.freePasses() > 0) {
        player.setFreePasses(player.freePasses() - 1);
        player.releaseFromLockup();
        std::cout << player.name() << " spends a Free-Pass Token to leave The Lockup\n";
        return true;
    }
    if (player.reagents() >= kLockupFine) {
        payTreasury(player, kLockupFine);
        player.releaseFromLockup();
        std::cout << player.name() << " pays the " << kLockupFine << "-Reagent fine\n";
        return true;
    }
    std::cout << player.name() << " cannot pay the " << kLockupFine << "-Reagent fine\n";
    return false;
}

void Game::payTreasury(Player &player, int amount) {
    player.addReagents(-amount);
    treasury_ += amount;
    std::cout << player.name() << " pays " << amount << " Reagents to the Guild Treasury\n";
}

void Game::payPlayer(Player &from, Player &to, int amount) {
    // TODO(insolvency step): when `from` cannot cover this, the debt should
    // open the raise-funds-or-declare-insolvency path instead of going negative.
    from.addReagents(-amount);
    to.addReagents(amount);
    std::cout << from.name() << " pays " << amount << " Reagents to " << to.name() << '\n';
}

bool Game::guildComplete(Guild guild, const Player &owner) const {
    int inGuild = 0;
    int held = 0;
    for (int square = 0; square < Board::size(); ++square) {
        const District &district = board_.at(square);
        if (district.kind() != DistrictKind::Workshop || district.guild() != guild) continue;
        ++inGuild;
        const auto *ownable = static_cast<const OwnableDistrict *>(&district);
        if (ownable->owner() == &owner) ++held;
    }
    return inGuild > 0 && inGuild == held;
}

int Game::countOwned(DistrictKind kind, const Player &owner) const {
    int total = 0;
    for (int square = 0; square < Board::size(); ++square) {
        const District &district = board_.at(square);
        if (district.kind() != kind) continue;
        const auto *ownable = dynamic_cast<const OwnableDistrict *>(&district);
        if (ownable && ownable->owner() == &owner) ++total;
    }
    return total;
}

int Game::totalWorth(const Player &player) const {
    int worth = player.reagents();
    for (int square = 0; square < Board::size(); ++square) {
        const auto *ownable = dynamic_cast<const OwnableDistrict *>(&board_.at(square));
        if (!ownable || ownable->owner() != &player) continue;
        worth += ownable->buyPrice();
        if (const auto *workshop = dynamic_cast<const Workshop *>(ownable)) {
            worth += workshop->upgradeCount() * workshop->upgradeCost();
        }
    }
    return worth;
}

std::string Game::prompt(const std::string &question) const {
    std::cout << question;
    std::cout.flush();
    std::string line;
    std::getline(std::cin, line);
    return line;
}
