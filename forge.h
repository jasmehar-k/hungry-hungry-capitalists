#ifndef FORGE_H
#define FORGE_H

#include <string>

#include "district.h"

// A Forge levies 25, 50, 100 or 200 Reagents according to how many of the four
// Forges its owner holds, so its toll depends on the rest of the board rather
// than on anything stored here. It takes no upgrades and belongs to no Guild,
// which is why it overrides computeToll and nothing else.
class Forge final : public OwnableDistrict {
public:
    Forge(int square, std::string name, int buyPrice);

    DistrictKind kind() const override { return DistrictKind::Forge; }

protected:
    int computeToll(const Game &game, const Player &visitor) const override;
};

#endif  // FORGE_H
