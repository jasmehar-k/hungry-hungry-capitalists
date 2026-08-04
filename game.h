#ifndef GAME_H
#define GAME_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "board.h"
#include "observer.h"

class Player;

// The Model. Holds all game state and knows nothing about how it is displayed;
// every mutating method ends by notifying observers.
class Game final : public Subject {
public:
    explicit Game(bool testingMode);
    ~Game() override;

    Board &board() { return board_; }
    const Board &board() const { return board_; }

    // Rejects a seventh Artificer or a token already in play, per the spec.
    bool addPlayer(std::string name, char token);
    bool tokenTaken(char token) const;
    std::size_t playerCount() const { return players_.size(); }
    const Player &player(std::size_t index) const;
    Player &player(std::size_t index);

    const Player &currentPlayer() const;
    Player &currentPlayer();
    std::size_t currentPlayerIndex() const { return current_; }
    void endTurn();

    bool testingMode() const { return testingMode_; }

private:
    Board board_;
    std::vector<std::unique_ptr<Player>> players_;
    std::size_t current_ = 0;
    bool testingMode_;
};

#endif  // GAME_H
