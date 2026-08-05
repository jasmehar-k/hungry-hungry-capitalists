#include "cards.h"

#include <algorithm>

// The two card squares, in both of the forms the project supports: the spec's
// probability tables, which are memoryless, and the optional shuffled deck,
// which is not. Keeping them in one file keeps the deck's contents and the
// table it must reproduce from drifting apart.

namespace {

// Walks the weighted table with a single uniform draw. The running subtraction
// gives each entry exactly `weight` of the `total` tickets.
template <typename Table>
const typename Table::value_type &pick(const Table &table, int total, std::mt19937 &rng) {
    std::uniform_int_distribution<int> tickets{0, total - 1};
    int ticket = tickets(rng);
    for (const auto &entry : table) {
        if (ticket < entry.weight) return entry;
        ticket -= entry.weight;
    }
    return table.back();  // unreachable while the weights sum to total
}

}  // namespace

const MarketOutcome &drawMarketOutcome(std::mt19937 &rng) {
    return pick(kMarketTable, kMarketWeightTotal, rng);
}

const EnchantmentOutcome &drawEnchantment(std::mt19937 &rng) {
    return pick(kEnchantmentTable, kEnchantmentWeightTotal, rng);
}

// The deck is dealt rather than sampled, so drawing a card changes the odds on
// the cards still in it. That is the whole difference from the table above,
// and it is why a full pass reproduces Table 3 exactly.
CardDeck::CardDeck(std::mt19937 &rng) : rng_{rng} { refill(); }

void CardDeck::refill() {
    // One card per weight unit, so a full pass deals exactly Table 3.
    cards_.clear();
    cards_.reserve(kEnchantmentWeightTotal);
    for (const EnchantmentOutcome &outcome : kEnchantmentTable) {
        for (int copy = 0; copy < outcome.weight; ++copy) {
            cards_.push_back(outcome.change);
        }
    }
    std::shuffle(cards_.begin(), cards_.end(), rng_);
    next_ = 0;
}

// Reshuffling happens only once the deck is spent, which is what the
// enhancement asks for: the cards run out before any of them can repeat.
int CardDeck::draw() {
    if (next_ >= cards_.size()) refill();
    return cards_[next_++];
}
