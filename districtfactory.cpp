#include "districtfactory.h"

#include <string>

#include "depot.h"
#include "district.h"
#include "forge.h"
#include "specialdistricts.h"
#include "workshop.h"

// One switch over a closed set, which is the Simple Factory idiom rather than
// the Factory Method pattern: there is no creator hierarchy and no subclass
// decides what gets built.
//
// A switch on a type tag is normally the thing to remove, but it is defensible
// at exactly one place in a program: the point where objects are created, since
// something must eventually name a concrete class. Everything downstream then
// works through District's virtual interface. Because the switch has no
// default label, adding a DistrictKind makes the compiler point here.
std::unique_ptr<District> DistrictFactory::create(const DistrictSpec &spec) {
    std::string name{spec.name};
    switch (spec.kind) {
        case DistrictKind::Workshop:
            return std::make_unique<Workshop>(spec.square, std::move(name), spec.guild,
                                              spec.buyPrice, spec.upgradeCost, spec.tolls);
        case DistrictKind::Forge:
            return std::make_unique<Forge>(spec.square, std::move(name), spec.buyPrice);
        case DistrictKind::Depot:
            return std::make_unique<Depot>(spec.square, std::move(name), spec.buyPrice);
        case DistrictKind::GrandEntrance:
            return std::make_unique<GrandEntrance>(spec.square, std::move(name));
        case DistrictKind::Lockup:
            return std::make_unique<Lockup>(spec.square, std::move(name));
        case DistrictKind::FreeRest:
            return std::make_unique<FreeRest>(spec.square, std::move(name));
        case DistrictKind::GoToLockup:
            return std::make_unique<GoToLockup>(spec.square, std::move(name));
        case DistrictKind::GuildLevy:
            return std::make_unique<GuildLevy>(spec.square, std::move(name));
        case DistrictKind::TransitToll:
            return std::make_unique<TransitToll>(spec.square, std::move(name));
        case DistrictKind::MarketFestival:
            return std::make_unique<MarketFestival>(spec.square, std::move(name));
        case DistrictKind::WanderingMarket:
            return std::make_unique<WanderingMarket>(spec.square, std::move(name));
        case DistrictKind::EnchantmentDeck:
            return std::make_unique<EnchantmentDeck>(spec.square, std::move(name));
    }
    // Unreachable: every enumerator is handled above and the compiler enforces
    // that. Present only because a function returning a value must, and
    // omitting it would trip -Wreturn-type.
    return nullptr;
}
