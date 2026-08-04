#ifndef TURN_H
#define TURN_H

#include <iosfwd>
#include <memory>
#include <string_view>

#include "turnstate.h"

class Game;
class Player;

// The State Context. It owns every state object for its whole life and swaps a
// non-owning pointer to select the active one, so transitions allocate nothing.
//
// This is a separate class rather than folding the states into Game because
// Game is already the Model; holding four more members and the turn's rules
// would push it well past one responsibility.
class Turn {
public:
    explicit Turn(Game &game);
    ~Turn();

    // Its states hold Turn& back-references, so a copy would point at the original.
    Turn(const Turn &) = delete;
    Turn &operator=(const Turn &) = delete;

    // Actions, each delegated straight to the active state.
    void roll(int die1, int die2) { state_->roll(die1, die2); }
    void next() { state_->next(); }
    std::string_view stateName() const { return state_->name(); }

    // Called when control passes to an Artificer: a detained one starts in The
    // Lockup, everyone else with a roll pending.
    void beginTurn();

    void setState(TurnState *state) { state_ = state; }
    TurnState *preRollState() const { return preRoll_.get(); }
    TurnState *postRollState() const { return postRoll_.get(); }
    TurnState *lockupState() const { return lockup_.get(); }

    // A trade may be offered before or after rolling, so the phase it
    // interrupted is remembered and restored once the offer is answered.
    void enterTradeResponse();
    void resumeAfterTrade();

    Game &game() { return game_; }

    // Forwarders so a state talks to its Context alone rather than reaching
    // through it (Law of Demeter): the summary chapter's a.getB().getC() shape.
    std::ostream &out();
    void endTurn();

    int consecutiveDoubles() const { return consecutiveDoubles_; }
    void countDouble() { ++consecutiveDoubles_; }
    void resetDoubles() { consecutiveDoubles_ = 0; }

private:
    Game &game_;
    std::unique_ptr<TurnState> preRoll_;
    std::unique_ptr<TurnState> postRoll_;
    std::unique_ptr<TurnState> lockup_;
    std::unique_ptr<TurnState> awaitingTrade_;
    TurnState *state_ = nullptr;   // non-owning view of one of the four above
    TurnState *resume_ = nullptr;  // the phase a trade offer interrupted
    int consecutiveDoubles_ = 0;
};

#endif  // TURN_H
