#include "turnstate.h"

#include <ostream>

#include "boarddata.h"
#include "dice.h"
#include "game.h"
#include "player.h"
#include "turn.h"

// The four states of a turn. Each one answers the same two actions, roll and
// next, differently, and the action that is not legal in a state is answered
// with an explanation rather than being silently ignored.
//
// The alternative was one roll() on Turn with a phase enum and a chain of
// conditionals inside it. Splitting the phases into classes means each phase's
// rules sit together and adding one cannot disturb the others.

namespace {

// Negative values mean the caller wants random dice; -testing supplies real ones.
void applyRoll(Game &game, int die1, int die2) {
    if (die1 >= 0 && die2 >= 0) {
        game.dice().set(die1, die2);
    } else {
        game.dice().roll();
    }
}

void announceRoll(const Game &game) {
    const Dice &dice = game.dice();
    game.out() << game.currentPlayer().name() << " rolls " << dice.first() << " and "
               << dice.second() << " (" << dice.total() << ")\n";
}

// The spec resolves a shortfall on the spot: raise funds or declare
// insolvency. Barring a roll while a debt stands keeps that promise, and is
// what stops one Artificer from stacking up two separate obligations.
bool blockedByDebt(Game &game, const Player &player) {
    if (!game.owes(player)) return false;
    game.out() << "settle your debt of " << game.amountOwed(player)
               << " Reagents or declare insolvency before rolling\n";
    return true;
}

}  // namespace

// The ordinary start of a turn: roll, move, and land on whatever is there.
// Doubles earn another roll, which is why this state can transition back to
// itself, and three of them in a row end the turn in The Lockup instead.
void PreRollState::roll(int die1, int die2) {
    Game &game = turn_.game();
    Player &player = game.currentPlayer();
    if (blockedByDebt(game, player)) return;

    applyRoll(game, die1, die2);
    announceRoll(game);

    const Dice &dice = game.dice();

    if (dice.isDoubles()) {
        turn_.countDouble();
        // The count lives on the Turn, not the Player, because it resets when
        // the turn ends rather than persisting across the game.
        if (turn_.consecutiveDoubles() >= kMaxConsecutiveDoubles) {
            // The third total is deliberately not moved.
            game.out() << "three doubles in a row: " << player.name() << " goes to The Lockup\n";
            game.sendToLockup(player);
            turn_.resetDoubles();
            turn_.setState(turn_.postRollState());
            return;
        }
    } else {
        turn_.resetDoubles();
    }

    game.movePlayer(player, dice.total());

    // Landing may itself have sent the Artificer away, which forfeits the
    // extra roll that doubles would otherwise grant.
    if (player.inLockup()) {
        turn_.setState(turn_.postRollState());
        return;
    }
    turn_.setState(dice.isDoubles() ? turn_.preRollState() : turn_.postRollState());
}

// Refusing rather than ignoring: an Artificer who types next too early has
// made a mistake worth telling them about.
void PreRollState::next() { turn_.out() << "you still have a roll pending\n"; }

// The roll is spent, so this is the state where the trading and property
// commands happen and the only way forward is to end the turn.
void PostRollState::roll(int, int) { turn_.out() << "you have already rolled this turn\n"; }

void PostRollState::next() { turn_.endTurn(); }

// A detained Artificer has three ways out and may take any of them on any
// turn, not only the third: roll doubles, pay the fine, or spend a Free-Pass
// Token. Only on the third turn does the choice stop being optional.
void LockupState::roll(int die1, int die2) {
    Game &game = turn_.game();
    Player &player = game.currentPlayer();
    if (blockedByDebt(game, player)) return;

    // Paying the fine or spending a token is available on any detained turn,
    // not only the third. Either way the Artificer then rolls and moves.
    if (game.offerLockupExit(player)) {
        applyRoll(game, die1, die2);
        announceRoll(game);
        game.movePlayer(player, game.dice().total());
        turn_.setState(turn_.postRollState());
        return;
    }

    applyRoll(game, die1, die2);
    announceRoll(game);

    const Dice &dice = game.dice();

    if (dice.isDoubles()) {
        game.out() << player.name() << " rolls doubles and leaves The Lockup\n";
        player.releaseFromLockup();
        game.movePlayer(player, dice.total());
        turn_.setState(turn_.postRollState());
        return;
    }

    player.serveLockupTurn();
    if (player.lockupTurns() >= kMaxLockupTurns) {
        // Third turn without doubles: the fine or a token is now compulsory.
        if (game.forceLockupExit(player)) {
            game.movePlayer(player, dice.total());
        }
    } else {
        game.out() << player.name() << " stays in The Lockup ("
                   << (kMaxLockupTurns - player.lockupTurns()) << " turns left)\n";
    }
    turn_.setState(turn_.postRollState());
}

void LockupState::next() { turn_.endTurn(); }

// Neither action of the interrupted turn is available until the offer is
// answered, so both funnel into the same reminder of who has to answer it.
void AwaitingTradeResponseState::roll(int, int) { next(); }

void AwaitingTradeResponseState::next() {
    Game &game = turn_.game();
    game.out() << game.tradeTargetName()
               << " must answer the trade offer first: accept or reject\n";
}
