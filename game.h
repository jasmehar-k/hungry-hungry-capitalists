#ifndef GAME_H
#define GAME_H

#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "board.h"
#include "dice.h"
#include "observer.h"

class OwnableDistrict;
class Player;
class Turn;

// The Model. Holds all game state and knows nothing about how it is displayed.
//
// Notification is deliberately NOT fired from each mutator. The spec redraws
// the whole board after every command, and one command can move an Artificer,
// pay a toll and change the turn state, so per-mutation notification would
// redraw several times for a single command. GameController notifies once when
// the command finishes, which is the real unit of change here.
class Game final : public Subject {
public:
    explicit Game(bool testingMode, unsigned seed = 0);
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
    void setCurrentPlayer(std::size_t index);
    void endTurn();

    bool testingMode() const { return testingMode_; }
    Dice &dice() { return dice_; }
    const Dice &dice() const { return dice_; }
    Turn &turn() { return *turn_; }
    const Turn &turn() const { return *turn_; }

    // Moves an Artificer, paying the Grand Entrance bonus when the move passes
    // over it, then resolves the destination district.
    void movePlayer(Player &player, int steps);
    void sendToLockup(Player &player);
    // Spends a Free-Pass Token or pays the fine. False when neither is possible.
    bool forceLockupExit(Player &player);
    // Reagents paid to the Guild Treasury leave play.
    void payTreasury(Player &player, int amount);
    void payPlayer(Player &from, Player &to, int amount);
    int treasury() const { return treasury_; }

    // True when `owner` holds every Workshop in the Guild. Guild sizes differ
    // (Smoke has 3, Gilded has 5), so this counts the Guild's real membership.
    bool guildComplete(Guild guild, const Player &owner) const;
    int countOwned(DistrictKind kind, const Player &owner) const;
    // Savings plus printed value of every district plus every upgrade bought.
    int totalWorth(const Player &player) const;

    // Asks the user a question during a landing that requires a decision.
    std::string prompt(const std::string &question) const;

    // Offers an unowned district to whoever landed on it; a decline, or an
    // Artificer who cannot afford it, sends the district to auction.
    void offerPurchase(OwnableDistrict &district, Player &buyer);
    void runAuction(OwnableDistrict &district);

private:
    Board board_;
    std::vector<std::unique_ptr<Player>> players_;
    std::size_t current_ = 0;
    bool testingMode_;
    int treasury_ = 0;
    std::mt19937 rng_;   // declared before dice_, which holds a reference to it
    Dice dice_;
    std::unique_ptr<Turn> turn_;
};

#endif  // GAME_H
