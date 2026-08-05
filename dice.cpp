#include "dice.h"

// The generator is injected rather than owned so that the whole game runs from
// the one sequence main seeded at startup: a given seed then replays an
// identical game, which is what makes a bug found in play reproducible.
Dice::Dice(std::mt19937 &rng) : rng_{rng} {}

void Dice::roll() {
    first_ = face_(rng_);
    second_ = face_(rng_);
}

// Used only by -testing, where the spec allows any non-negative values rather
// than just 1 to 6. Totals larger than 12 are therefore possible, which is why
// Board::grandEntrancePasses counts laps instead of returning a bool.
void Dice::set(int die1, int die2) {
    first_ = die1;
    second_ = die2;
}
