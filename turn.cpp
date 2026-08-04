#include "turn.h"

#include "game.h"
#include "player.h"

Turn::Turn(Game &game)
    : game_{game},
      preRoll_{std::make_unique<PreRollState>(*this)},
      postRoll_{std::make_unique<PostRollState>(*this)},
      lockup_{std::make_unique<LockupState>(*this)},
      state_{preRoll_.get()} {}

Turn::~Turn() = default;

void Turn::beginTurn() {
    resetDoubles();
    if (game_.playerCount() == 0) return;
    state_ = game_.currentPlayer().inLockup() ? lockup_.get() : preRoll_.get();
}
