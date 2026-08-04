#ifndef CS247_BOARDDATA_H
#define CS247_BOARDDATA_H

#include <array>
#include <string_view>

// The single source of truth for the circuit. Every other module reads the
// board from here, so an error in this table propagates everywhere.
//
// Two details the spec's own figure gets wrong, resolved in CLAUDE.md:
//   - square 14 is Westgate Depot, not a Forge (Forges are 4, 18, 24, 29)
//   - Veil-watch buys at 170 (Table 1), not the figure's 180
// Guild sizes differ: Ember 4, Smoke 3, Arcane 4, Gilded 5.

inline constexpr int kBoardSize = 40;
inline constexpr int kGrandEntranceSquare = 0;
inline constexpr int kLockupSquare = 10;
inline constexpr int kFreeRestSquare = 20;
inline constexpr int kGoToLockupSquare = 30;

inline constexpr int kGrandEntranceBonus = 200;
inline constexpr int kMaxUpgrades = 5;

enum class Guild { None, Ember, Smoke, Arcane, Gilded };

enum class DistrictKind {
    Workshop,
    Forge,
    Depot,
    GrandEntrance,
    Lockup,
    FreeRest,
    GoToLockup,
    GuildLevy,
    TransitToll,
    MarketFestival,
    WanderingMarket,
    EnchantmentDeck
};

struct DistrictSpec {
    int square;
    std::string_view name;  // the save-file name, which is canonical
    DistrictKind kind;
    Guild guild;
    int buyPrice;               // 0 for non-ownable districts
    int upgradeCost;            // Workshops only
    std::array<int, 6> tolls;   // toll by upgrade level 0..5; Workshops only
};

namespace detail {
inline constexpr std::array<int, 6> kNoTolls{0, 0, 0, 0, 0, 0};
}

inline constexpr std::array<DistrictSpec, kBoardSize> kBoard{{
    {0, "Grand Entrance", DistrictKind::GrandEntrance, Guild::None, 0, 0, detail::kNoTolls},
    {1, "Tinder", DistrictKind::Workshop, Guild::Ember, 60, 50, {2, 10, 30, 90, 160, 250}},
    {2, "Flint", DistrictKind::Workshop, Guild::Ember, 60, 50, {4, 20, 60, 180, 320, 450}},
    {3, "Guild Levy", DistrictKind::GuildLevy, Guild::None, 0, 0, detail::kNoTolls},
    {4, "Ironhold", DistrictKind::Forge, Guild::None, 200, 0, detail::kNoTolls},
    {5, "Wandering Market", DistrictKind::WanderingMarket, Guild::None, 0, 0, detail::kNoTolls},
    {6, "Cinder-stone", DistrictKind::Workshop, Guild::Ember, 100, 50, {6, 30, 90, 270, 400, 550}},
    {7, "Enchantment Deck", DistrictKind::EnchantmentDeck, Guild::None, 0, 0, detail::kNoTolls},
    {8, "Slagpit", DistrictKind::Workshop, Guild::Ember, 120, 50, {8, 40, 100, 300, 450, 600}},
    {9, "Eastgate", DistrictKind::Depot, Guild::None, 150, 0, detail::kNoTolls},
    {10, "The Lockup", DistrictKind::Lockup, Guild::None, 0, 0, detail::kNoTolls},
    {11, "Ash-veil", DistrictKind::Workshop, Guild::Smoke, 140, 100, {10, 50, 150, 450, 625, 750}},
    {12, "Market Festival", DistrictKind::MarketFestival, Guild::None, 0, 0, detail::kNoTolls},
    {13, "Coal-gate", DistrictKind::Workshop, Guild::Smoke, 140, 100, {10, 50, 150, 450, 625, 750}},
    {14, "Westgate", DistrictKind::Depot, Guild::None, 150, 0, detail::kNoTolls},
    {15, "Wandering Market", DistrictKind::WanderingMarket, Guild::None, 0, 0, detail::kNoTolls},
    {16, "Mist-forge", DistrictKind::Workshop, Guild::Smoke, 160, 100, {12, 60, 180, 500, 700, 900}},
    {17, "Guild Levy", DistrictKind::GuildLevy, Guild::None, 0, 0, detail::kNoTolls},
    {18, "Bronze-works", DistrictKind::Forge, Guild::None, 200, 0, detail::kNoTolls},
    {19, "Enchantment Deck", DistrictKind::EnchantmentDeck, Guild::None, 0, 0, detail::kNoTolls},
    {20, "Free Rest", DistrictKind::FreeRest, Guild::None, 0, 0, detail::kNoTolls},
    {21, "Veil-watch", DistrictKind::Workshop, Guild::Arcane, 170, 150, {14, 70, 200, 550, 750, 950}},
    {22, "Rune-gate", DistrictKind::Workshop, Guild::Arcane, 180, 150, {14, 70, 200, 550, 750, 950}},
    {23, "Transit Toll", DistrictKind::TransitToll, Guild::None, 0, 0, detail::kNoTolls},
    {24, "Copper-gate", DistrictKind::Forge, Guild::None, 200, 0, detail::kNoTolls},
    {25, "Spell-wright", DistrictKind::Workshop, Guild::Arcane, 200, 150, {16, 80, 220, 600, 800, 1000}},
    {26, "Enchantment Deck", DistrictKind::EnchantmentDeck, Guild::None, 0, 0, detail::kNoTolls},
    {27, "Hex-ward", DistrictKind::Workshop, Guild::Arcane, 220, 150, {18, 90, 250, 700, 875, 1050}},
    {28, "Wandering Market", DistrictKind::WanderingMarket, Guild::None, 0, 0, detail::kNoTolls},
    {29, "Steel-spire", DistrictKind::Forge, Guild::None, 200, 0, detail::kNoTolls},
    {30, "Go to Lockup", DistrictKind::GoToLockup, Guild::None, 0, 0, detail::kNoTolls},
    {31, "Cinder-keep", DistrictKind::Workshop, Guild::Gilded, 220, 200, {18, 90, 250, 700, 875, 1050}},
    {32, "Enchantment Deck", DistrictKind::EnchantmentDeck, Guild::None, 0, 0, detail::kNoTolls},
    {33, "Gold-vault", DistrictKind::Workshop, Guild::Gilded, 240, 200, {20, 100, 300, 750, 925, 1100}},
    {34, "Transit Toll", DistrictKind::TransitToll, Guild::None, 0, 0, detail::kNoTolls},
    {35, "Amber-hall", DistrictKind::Workshop, Guild::Gilded, 240, 200, {20, 100, 300, 750, 925, 1100}},
    {36, "Wandering Market", DistrictKind::WanderingMarket, Guild::None, 0, 0, detail::kNoTolls},
    {37, "Nexus", DistrictKind::Workshop, Guild::Gilded, 260, 200, {26, 130, 390, 900, 1100, 1275}},
    {38, "Guild Levy", DistrictKind::GuildLevy, Guild::None, 0, 0, detail::kNoTolls},
    {39, "Apex", DistrictKind::Workshop, Guild::Gilded, 260, 200, {35, 175, 500, 1100, 1300, 1500}},
}};

// Forge levy by how many Forges the owner controls (1-based index into [1..4]).
inline constexpr std::array<int, 5> kForgeLevy{0, 25, 50, 100, 200};

// Depot levy multiplies the dice total; 10x when one Artificer owns both.
inline constexpr int kDepotSingleMultiplier = 4;
inline constexpr int kDepotBothMultiplier = 10;

#endif  // CS247_BOARDDATA_H
