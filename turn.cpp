#include "turn.h"

#include "game.h"
#include "player.h"

Turn::Turn(Game &game)
    : game_{game},
      preRoll_{std::make_unique<PreRollState>(*this)},
      postRoll_{std::make_unique<PostRollState>(*this)},
      lockup_{std::make_unique<LockupState>(*this)},
      awaitingTrade_{std::make_unique<AwaitingTradeResponseState>(*this)},
      state_{preRoll_.get()} {}

Turn::~Turn() = default;

std::ostream &Turn::out() { return game_.out(); }

void Turn::endTurn() { game_.endTurn(); }

void Turn::beginTurn() {
    resetDoubles();
    resume_ = nullptr;
    if (game_.playerCount() == 0) return;
    state_ = game_.currentPlayer().inLockup() ? lockup_.get() : preRoll_.get();
}

void Turn::enterTradeResponse() {
    if (state_ == awaitingTrade_.get()) return;  // already awaiting an answer
    resume_ = state_;
    state_ = awaitingTrade_.get();
}

void Turn::resumeAfterTrade() {
    if (resume_ == nullptr) return;
    state_ = resume_;
    resume_ = nullptr;
}
