#ifndef DEPOT_H
#define DEPOT_H

#include <string>

#include "district.h"

// A Depot charges four times the dice total, or ten times it if one Artificer
// holds both Depots. Its toll is therefore the only one that depends on the
// roll that brought the visitor here rather than on the district alone.
class Depot final : public OwnableDistrict {
public:
    Depot(int square, std::string name, int buyPrice);

    DistrictKind kind() const override { return DistrictKind::Depot; }

protected:
    int computeToll(const Game &game, const Player &visitor) const override;
};

#endif  // DEPOT_H
