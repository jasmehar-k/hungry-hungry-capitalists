#ifndef CS247_DEPOT_H
#define CS247_DEPOT_H

#include <string>

#include "district.h"

class Depot final : public OwnableDistrict {
public:
    Depot(int square, std::string name, int buyPrice);

    DistrictKind kind() const override { return DistrictKind::Depot; }

protected:
    int computeToll(const Game &game, const Player &visitor) const override;
};

#endif  // CS247_DEPOT_H
