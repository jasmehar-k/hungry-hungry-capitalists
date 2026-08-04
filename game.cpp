#include "game.h"

#include <cassert>
#include <iostream>
#include <utility>

#include "boarddata.h"
#include "district.h"
#include "player.h"
#include "turn.h"

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
}
