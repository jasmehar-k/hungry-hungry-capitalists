#include "game.h"

#include <cassert>
#include <utility>

#include "player.h"

Game::Game(bool testingMode) : testingMode_{testingMode} {}

Game::~Game() = default;

void Game::addPlayer(std::string name, char token) {
    players_.push_back(std::make_unique<Player>(std::move(name), token));
    notifyObservers();
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

void Game::endTurn() {
    if (players_.empty()) return;
    current_ = (current_ + 1) % players_.size();
    notifyObservers();
}
