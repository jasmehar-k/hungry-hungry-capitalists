#ifndef DICE_H
#define DICE_H

#include <random>

// Two six-sided dice. Under -testing the values may be set explicitly, and the
// spec allows any non-negative integer there, not just 1-6.
class Dice {
public:
    explicit Dice(std::mt19937 &rng);

    void roll();
    void set(int die1, int die2);

    int first() const { return first_; }
    int second() const { return second_; }
    int total() const { return first_ + second_; }
    bool isDoubles() const { return first_ == second_; }

private:
    std::mt19937 &rng_;
    std::uniform_int_distribution<int> face_{1, 6};
    int first_ = 0;
    int second_ = 0;
};

#endif  // DICE_H
