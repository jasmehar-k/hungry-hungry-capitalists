#ifndef CS247_WORKSHOP_H
#define CS247_WORKSHOP_H

#include <array>
#include <string>

#include "district.h"

class Workshop final : public OwnableDistrict {
public:
    Workshop(int square, std::string name, Guild guild, int buyPrice, int upgradeCost,
             const std::array<int, 6> &tolls);

    DistrictKind kind() const override { return DistrictKind::Workshop; }
    Guild guild() const override { return guild_; }

    int upgradeCount() const override { return upgrades_; }
    void setUpgrades(int count) { upgrades_ = count; }
    int upgradeCost() const { return upgradeCost_; }
    int tollAt(int level) const;

protected:
    int computeToll(const Game &game, const Player &visitor) const override;

private:
    Guild guild_;
    int upgradeCost_;
    std::array<int, 6> tolls_;
    int upgrades_ = 0;
};

#endif  // CS247_WORKSHOP_H
