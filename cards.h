#ifndef CARDS_H
#define CARDS_H

#include <array>
#include <cstddef>
#include <random>
#include <string_view>
#include <vector>

// The two random-effect tables from the spec. Both sets of probabilities have
// exact integer weights, so they are stored as weights rather than as floating
// point: Table 2 is twenty-fourths, Table 3 is eighteenths.
//
// A mistyped weight would be nearly invisible in play, so the sums are checked
// at compile time and the distributions are checked by the test harness.

// Table 2, Wandering Market movements (spec p. 5).
enum class MarketMove {
    Back3,
    Back2,
    Back1,
    Forward1,
    Forward2,
    Forward3,
    ToLockup,
    ToGrandEntrance
};

struct MarketOutcome {
    MarketMove move;
    int weight;
    std::string_view description;
};

inline constexpr int kMarketWeightTotal = 24;

inline constexpr std::array<MarketOutcome, 8> kMarketTable{{
    {MarketMove::Back3, 3, "back 3"},
    {MarketMove::Back2, 4, "back 2"},
    {MarketMove::Back1, 4, "back 1"},
    {MarketMove::Forward1, 3, "forward 1"},
    {MarketMove::Forward2, 4, "forward 2"},
    {MarketMove::Forward3, 4, "forward 3"},
    {MarketMove::ToLockup, 1, "go to The Lockup"},
    {MarketMove::ToGrandEntrance, 1, "advance to the Grand Entrance"},
}};

// Table 3, Enchantment Deck effects (spec p. 6).
struct EnchantmentOutcome {
    int change;
    int weight;
};

inline constexpr int kEnchantmentWeightTotal = 18;

inline constexpr std::array<EnchantmentOutcome, 7> kEnchantmentTable{{
    {-200, 1},
    {-100, 2},
    {-50, 3},
    {25, 6},
    {50, 3},
    {100, 2},
    {200, 1},
}};

constexpr int sumMarketWeights() {
    int total = 0;
    for (const MarketOutcome &outcome : kMarketTable) total += outcome.weight;
    return total;
}

constexpr int sumEnchantmentWeights() {
    int total = 0;
    for (const EnchantmentOutcome &outcome : kEnchantmentTable) total += outcome.weight;
    return total;
}

static_assert(sumMarketWeights() == kMarketWeightTotal, "Table 2 must sum to 24/24");
static_assert(sumEnchantmentWeights() == kEnchantmentWeightTotal, "Table 3 must sum to 18/18");

// Taking the generator by reference keeps these directly testable: the harness
// draws from them a hundred thousand times without constructing a Game.
const MarketOutcome &drawMarketOutcome(std::mt19937 &rng);
const EnchantmentOutcome &drawEnchantment(std::mt19937 &rng);

// The optional enhancement: a real deck rather than a distribution.
//
// These are not the same thing. Drawing from `drawEnchantment` is memoryless,
// so -200 can come up twice running. A deck is stateful: each card leaves the
// deck when drawn, which shifts the odds on every later draw and makes counting
// cards a real strategy. Over one full pass the counts match Table 3 exactly,
// which a distribution only approaches in the limit.
//
// The deck holds one card per weight unit, so eighteen cards in total.
class CardDeck {
public:
    explicit CardDeck(std::mt19937 &rng);

    // Draws the next card, reshuffling a fresh deck once this one runs out.
    int draw();

    std::size_t remaining() const { return cards_.size() - next_; }
    static std::size_t size() { return kEnchantmentWeightTotal; }

private:
    void refill();

    std::mt19937 &rng_;
    std::vector<int> cards_;
    std::size_t next_ = 0;
};

#endif  // CARDS_H
