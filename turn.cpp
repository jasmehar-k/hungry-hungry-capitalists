#include "turn.h"

#include "game.h"
#include "player.h"

// Every state is built once, here, and lives for as long as the Turn does.
// Transitions then only reseat a pointer, so no state object is ever allocated
// or destroyed mid-game and a transition cannot fail.
Turn::Turn(Game &game)
    : game_{game},
      preRoll_{std::make_unique<PreRollState>(*this)},
      postRoll_{std::make_unique<PostRollState>(*this)},
      lockup_{std::make_unique<LockupState>(*this)},
      awaitingTrade_{std::make_unique<AwaitingTradeResponseState>(*this)},
      state_{preRoll_.get()} {}

// Out of line because the unique_ptr members need TurnState to be complete
// where the destructor is generated.
Turn::~Turn() = default;

std::ostream &Turn::out() { return game_.out(); }

void Turn::endTurn() { game_.endTurn(); }

// Control passing to an Artificer decides which state they start in, which is
// the only place that choice is made: a detained Artificer resumes in The
// Lockup, everyone else with a roll pending. Loading a saved game reaches here
// too, so a detained Artificer cannot be loaded straight into a free roll.
void Turn::beginTurn() {
    resetDoubles();
    resume_ = nullptr;
    if (game_.playerCount() == 0) return;
    state_ = game_.currentPlayer().inLockup() ? lockup_.get() : preRoll_.get();
}

// A trade may be offered before or after rolling, so the interrupted phase is
// remembered rather than assumed. Guarding on the current state keeps a second
// offer from overwriting the phase to return to with the trade state itself.
void Turn::enterTradeResponse() {
    if (state_ == awaitingTrade_.get()) return;  // already awaiting an answer
    resume_ = state_;
    state_ = awaitingTrade_.get();
}

// Both accept and reject come back through here, because answering an offer
// either way returns the turn to exactly where it was.
void Turn::resumeAfterTrade() {
    if (resume_ == nullptr) return;
    state_ = resume_;
    resume_ = nullptr;
}
