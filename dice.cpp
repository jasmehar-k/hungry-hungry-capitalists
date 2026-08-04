#include "dice.h"

Dice::Dice(std::mt19937 &rng) : rng_{rng} {}

void Dice::roll() {
    first_ = face_(rng_);
    second_ = face_(rng_);
}

void Dice::set(int die1, int die2) {
    first_ = die1;
    second_ = die2;
}
