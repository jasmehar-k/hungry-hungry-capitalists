#ifndef SPECIALDISTRICTS_H
#define SPECIALDISTRICTS_H

#include <string>
#include <utility>

#include "district.h"

// The eighteen squares nobody can own. They share District's interface but not
// OwnableDistrict's Template Method, because there is no purchase, no owner
// and no Toll Fee to compute — each simply does its own thing on arrival.
//
// They are separate classes rather than one class with a kind field so that
// landOn is chosen by virtual dispatch instead of by a switch, which is what
// lets Board hold all forty squares as District pointers and resolve a landing
// without asking any of them what they are.

// The districts the Guild administers. They cannot be bought, so they keep
// District's defaults for ownership and upgrades. Each resolves a visit
// differently, which is why these are separate classes rather than one class
// carrying a kind flag: the difference is behaviour, not data.

class GrandEntrance final : public District {
public:
    GrandEntrance(int square, std::string name) : District{square, std::move(name)} {}
    DistrictKind kind() const override { return DistrictKind::GrandEntrance; }
    void landOn(Game &game, Player &visitor) override;
};

class Lockup final : public District {
public:
    Lockup(int square, std::string name) : District{square, std::move(name)} {}
    DistrictKind kind() const override { return DistrictKind::Lockup; }
    void landOn(Game &game, Player &visitor) override;
};

class FreeRest final : public District {
public:
    FreeRest(int square, std::string name) : District{square, std::move(name)} {}
    DistrictKind kind() const override { return DistrictKind::FreeRest; }
    void landOn(Game &game, Player &visitor) override;
};

class GoToLockup final : public District {
public:
    GoToLockup(int square, std::string name) : District{square, std::move(name)} {}
    DistrictKind kind() const override { return DistrictKind::GoToLockup; }
    void landOn(Game &game, Player &visitor) override;
};

class GuildLevy final : public District {
public:
    GuildLevy(int square, std::string name) : District{square, std::move(name)} {}
    DistrictKind kind() const override { return DistrictKind::GuildLevy; }
    void landOn(Game &game, Player &visitor) override;
};

class TransitToll final : public District {
public:
    TransitToll(int square, std::string name) : District{square, std::move(name)} {}
    DistrictKind kind() const override { return DistrictKind::TransitToll; }
    void landOn(Game &game, Player &visitor) override;
};

class MarketFestival final : public District {
public:
    MarketFestival(int square, std::string name) : District{square, std::move(name)} {}
    DistrictKind kind() const override { return DistrictKind::MarketFestival; }
    void landOn(Game &game, Player &visitor) override;
};

class WanderingMarket final : public District {
public:
    WanderingMarket(int square, std::string name) : District{square, std::move(name)} {}
    DistrictKind kind() const override { return DistrictKind::WanderingMarket; }
    void landOn(Game &game, Player &visitor) override;
};

class EnchantmentDeck final : public District {
public:
    EnchantmentDeck(int square, std::string name) : District{square, std::move(name)} {}
    DistrictKind kind() const override { return DistrictKind::EnchantmentDeck; }
    void landOn(Game &game, Player &visitor) override;
};

#endif  // SPECIALDISTRICTS_H
