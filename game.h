#ifndef GAME_H
#define GAME_H

#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "board.h"
#include "cards.h"
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
    // shuffledDeck selects the optional enhancement: Enchantment effects come
    // from a real deck rather than the spec's probability table. It is chosen
    // at startup so the game plays either way without recompiling.
    explicit Game(bool testingMode, unsigned seed = 0, bool shuffledDeck = false);
    ~Game() override;

    // The single Model instance; observers and districts hold pointers into it.
    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;

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
    // The card squares draw from their own tables, so they need the generator
    // that main seeded once at startup.
    std::mt19937 &rng() { return rng_; }
    Dice &dice() { return dice_; }
    const Dice &dice() const { return dice_; }
    Turn &turn() { return *turn_; }
    const Turn &turn() const { return *turn_; }

    // Moves an Artificer, paying the Grand Entrance bonus when the move passes
    // over it, then resolves the destination district.
    void movePlayer(Player &player, int steps);
    void sendToLockup(Player &player);
    // The spec offers a detained Artificer three routes out on every turn, not
    // only the third: roll for doubles, pay the fine, or spend a token. Returns
    // true when they bought their way out, in which case they still roll and
    // move. Asks nothing when neither the fine nor a token is affordable.
    bool offerLockupExit(Player &player);
    // The third turn, where the fine or a token stops being optional.
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

    // The single seam between the two Enchantment implementations: everything
    // else about the square is identical either way.
    int drawEnchantmentValue();
    bool usingShuffledDeck() const { return deck_ != nullptr; }

    int totalFreePasses() const;
    // The 1% draw that replaces a card square's normal effect. Returns false
    // once four tokens are in play, which is the spec's hard cap. Both card
    // squares call this, so the cap cannot drift between them.
    bool tryGrantFreePass(Player &player);

    // Asks the user a question during a landing that requires a decision.
    std::string prompt(const std::string &question) const;

    // Offers an unowned district to whoever landed on it; a decline, or an
    // Artificer who cannot afford it, sends the district to auction.
    void offerPurchase(OwnableDistrict &district, Player &buyer);
    void runAuction(OwnableDistrict &district);

    // A payment larger than the debtor holds is parked rather than driving the
    // balance negative: the spec gives them the choice of raising funds or
    // declaring insolvency, and no turn may end while a debt is outstanding.
    bool hasPendingDebt() const { return debtAmount_ > 0; }
    void declareInsolvency();

    bool mortgage(const std::string &districtName);
    bool unmortgage(const std::string &districtName);

    // Upgrades are Workshops only, and only in a Guild the Artificer owns
    // outright. Selling one is a legitimate way to raise funds against a debt.
    bool buyUpgrade(const std::string &districtName);
    bool sellUpgrade(const std::string &districtName);

    // give and receive are each either a Reagent amount or a district name.
    // Reagent-for-Reagent is rejected, and a district may only move while every
    // district in its Guild is unimproved.
    bool offerTrade(const std::string &targetName, const std::string &give,
                    const std::string &receive);
    void acceptTrade();
    void rejectTrade();
    bool hasTradeOffer() const { return offer_.active; }
    std::string tradeTargetName() const;

    std::size_t solventCount() const;
    const Player *winner() const;
    bool isOver() const { return winner() != nullptr; }

    void printAssets(const Player &player) const;
    void printAllAssets() const;

private:
    Board board_;
    std::vector<std::unique_ptr<Player>> players_;
    std::size_t current_ = 0;
    bool testingMode_;
    int treasury_ = 0;
    std::mt19937 rng_;   // declared before dice_, which holds a reference to it
    Dice dice_;
    std::unique_ptr<CardDeck> deck_;  // null unless the enhancement is on
    std::unique_ptr<Turn> turn_;

    // The outstanding debt, if any. A null creditor means the Guild Treasury.
    Player *debtor_ = nullptr;
    Player *creditor_ = nullptr;
    int debtAmount_ = 0;

    // When the compulsory Lockup fine cannot be paid it becomes a debt, and
    // settling that debt must still finish the exit: release the Artificer and
    // move them the total they rolled on the turn the fine fell due.
    bool lockupFinePending_ = false;
    int lockupFineMove_ = 0;

    // A district is either given or asked for; a null pointer on a side means
    // that side is Reagents instead.
    struct TradeOffer {
        Player *from = nullptr;
        Player *to = nullptr;
        OwnableDistrict *giveDistrict = nullptr;
        int giveReagents = 0;
        OwnableDistrict *receiveDistrict = nullptr;
        int receiveReagents = 0;
        bool active = false;
    };
    TradeOffer offer_;

    void settleDebtIfPossible();
    // Hands a district over and *returns* the 10% fee due on a mortgaged one
    // rather than charging it, so a caller moving several districts can settle
    // them in one payment instead of parking several competing debts.
    int transferDistrict(OwnableDistrict &district, Player &to);
    static int mortgageTransferFee(const OwnableDistrict &district);
    void transferEverything(Player &from, Player &to);
    void releaseToMarket(Player &from);
    bool guildIsUnimproved(Guild guild) const;
};

#endif  // GAME_H
