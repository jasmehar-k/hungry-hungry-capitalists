// Statistical check of the two probability tables and the shuffled deck,
// built and run with `make test`.
//
// This is the one part of the project that playing the game cannot verify: a
// mistyped weight still produces plausible-looking output forever. The weights
// are already asserted to sum to 24 and 18 at compile time; this harness draws
// 200,000 outcomes from each table and compares observed frequencies against
// the spec to within 0.004 (roughly nine standard deviations, so a correct
// table never trips it and a transposed digit always does), then deals the
// CardDeck through 100 consecutive passes and requires every pass to match
// Table 3 exactly, card for card.

#include <cmath>
#include <cstddef>
#include <iostream>
#include <map>
#include <random>

#include "cards.h"

namespace {

constexpr int kDraws = 200000;
constexpr double kTolerance = 0.004;

int failures = 0;

void checkFrequency(const char *what, int seen, int weight, int total) {
    const double observed = static_cast<double>(seen) / kDraws;
    const double expected = static_cast<double>(weight) / total;
    if (std::abs(observed - expected) > kTolerance) {
        std::cout << "FAIL: " << what << " observed " << observed << ", expected " << expected
                  << '\n';
        ++failures;
    }
}

void testMarketTable(std::mt19937 &rng) {
    std::map<MarketMove, int> seen;
    for (int i = 0; i < kDraws; ++i) seen[drawMarketOutcome(rng).move]++;
    for (const MarketOutcome &outcome : kMarketTable) {
        checkFrequency("Table 2 (Wandering Market)", seen[outcome.move], outcome.weight,
                       kMarketWeightTotal);
    }
}

void testEnchantmentTable(std::mt19937 &rng) {
    std::map<int, int> seen;
    for (int i = 0; i < kDraws; ++i) seen[drawEnchantment(rng).change]++;
    for (const EnchantmentOutcome &outcome : kEnchantmentTable) {
        checkFrequency("Table 3 (Enchantment)", seen[outcome.change], outcome.weight,
                       kEnchantmentWeightTotal);
    }
}

// Every full pass of the deck must deal Table 3 exactly — that exactness is
// what distinguishes a real deck from a reshuffled distribution. Passes after
// the first prove each refill deals a full fresh deck rather than a partial one.
void testDeckDealsTableExactly(std::mt19937 &rng) {
    CardDeck deck{rng};
    for (int pass = 0; pass < 100; ++pass) {
        std::map<int, int> seen;
        for (std::size_t i = 0; i < CardDeck::size(); ++i) seen[deck.draw()]++;
        for (const EnchantmentOutcome &outcome : kEnchantmentTable) {
            if (seen[outcome.change] != outcome.weight) {
                std::cout << "FAIL: deck pass " << pass << " dealt " << seen[outcome.change]
                          << " copies of " << outcome.change << ", expected " << outcome.weight
                          << '\n';
                ++failures;
            }
        }
    }
}

}  // namespace

int main() {
    std::mt19937 rng{247};  // fixed seed: the check must be reproducible
    testMarketTable(rng);
    testEnchantmentTable(rng);
    testDeckDealsTableExactly(rng);

    if (failures == 0) {
        std::cout << "tabletest: all checks passed (" << kDraws
                  << " draws per table, tolerance " << kTolerance << ")\n";
        return 0;
    }
    std::cout << "tabletest: " << failures << " check(s) failed\n";
    return 1;
}
