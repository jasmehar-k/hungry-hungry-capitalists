#include "turnstate.h"

#include <iostream>

#include "boarddata.h"
#include "dice.h"
#include "game.h"
#include "player.h"
#include "turn.h"

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
    std::cout << game.currentPlayer().name() << " rolls " << dice.first() << " and "
              << dice.second() << " (" << dice.total() << ")\n";
}

}  // namespace

void PreRollState::roll(int die1, int die2) {
    Game &game = turn_.game();
    applyRoll(game, die1, die2);
    announceRoll(game);

    Player &player = game.currentPlayer();
    const Dice &dice = game.dice();

    if (dice.isDoubles()) {
        turn_.countDouble();
        if (turn_.consecutiveDoubles() >= kMaxConsecutiveDoubles) {
            // The third total is deliberately not moved.
            std::cout << "three doubles in a row: " << player.name() << " goes to The Lockup\n";
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

void PreRollState::next() { std::cout << "you still have a roll pending\n"; }

void PostRollState::roll(int, int) { std::cout << "you have already rolled this turn\n"; }

void PostRollState::next() { turn_.game().endTurn(); }

void LockupState::roll(int die1, int die2) {
    Game &game = turn_.game();
    Player &player = game.currentPlayer();

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
        std::cout << player.name() << " rolls doubles and leaves The Lockup\n";
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
        std::cout << player.name() << " stays in The Lockup ("
                  << (kMaxLockupTurns - player.lockupTurns()) << " turns left)\n";
    }
    turn_.setState(turn_.postRollState());
}

void LockupState::next() { turn_.game().endTurn(); }

void AwaitingTradeResponseState::roll(int, int) { next(); }

void AwaitingTradeResponseState::next() {
    std::cout << turn_.game().tradeTargetName()
              << " must answer the trade offer first: accept or reject\n";
}
