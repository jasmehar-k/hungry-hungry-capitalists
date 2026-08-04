#include "cards.h"

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
