#ifndef WORKSHOP_H
#define WORKSHOP_H

#include <array>
#include <string>

#include "district.h"

// A Workshop is the only district that can be improved. It is one of the three
// concrete classes under OwnableDistrict's Template Method: landOn is fixed in
// the base and each subclass supplies only computeToll, which is the single
// step that actually differs between a Workshop, a Forge and a Depot.
class Workshop final : public OwnableDistrict {
public:
    Workshop(int square, std::string name, Guild guild, int buyPrice, int upgradeCost,
             const std::array<int, 6> &tolls);

    DistrictKind kind() const override { return DistrictKind::Workshop; }
    Guild guild() const override { return guild_; }

    int upgradeCount() const override { return upgrades_; }
    // The rules around buying and selling live in Game, which is what can see
    // the Guild and the Artificer's Reagents; save/load also restores a level
    // directly. This setter is deliberately unguarded for that reason.
    void setUpgrades(int count) { upgrades_ = count; }
    int upgradeCost() const { return upgradeCost_; }
    // What the upgrades on this district are worth, which total worth and the
    // insolvency settlement both need. Declared on the base so those sweeps
    // never have to ask whether a district happens to be a Workshop.
    int upgradeInvestment() const override { return upgrades_ * upgradeCost_; }
    int tollAt(int level) const;

protected:
    int computeToll(const Game &game, const Player &visitor) const override;

private:
    Guild guild_;
    int upgradeCost_;
    // Six entries: the unimproved toll followed by one per upgrade level, so
    // the level indexes the table directly. This is why upgrades are a counter
    // rather than a chain of Decorators — see Design Question 3.
    std::array<int, 6> tolls_;
    int upgrades_ = 0;
};

#endif  // WORKSHOP_H
