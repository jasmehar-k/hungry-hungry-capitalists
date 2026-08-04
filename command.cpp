#include "command.h"

#include "game.h"
#include "turn.h"

void RollCommand::execute() { game_.turn().roll(die1_, die2_); }

void NextCommand::execute() { game_.turn().next(); }
