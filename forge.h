#ifndef FORGE_H
#define FORGE_H

#include <string>

#include "district.h"

class Forge final : public OwnableDistrict {
public:
    Forge(int square, std::string name, int buyPrice);

    DistrictKind kind() const override { return DistrictKind::Forge; }

protected:
    int computeToll(const Game &game, const Player &visitor) const override;
};

#endif  // FORGE_H
