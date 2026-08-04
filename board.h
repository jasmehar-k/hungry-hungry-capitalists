#ifndef CS247_BOARD_H
#define CS247_BOARD_H

#include <memory>
#include <string>
#include <vector>

#include "boarddata.h"

class District;
class OwnableDistrict;

// The circuit. Owns all 40 districts and hands out references to them.
// Play runs clockwise: square index increasing, wrapping 39 -> 0.
class Board {
public:
    Board();
    ~Board();

    static constexpr int size() { return kBoardSize; }

    const District &at(int square) const;
    District &at(int square);

    // Advance a position by a number of squares, wrapping the circuit.
    static int advance(int from, int steps);
    // True when moving from one square to another passed over Grand Entrance.
    static bool passedGrandEntrance(int from, int to);

    // Ownable district names are unique across the circuit, so this is the
    // lookup the save file and the district-naming commands need.
    OwnableDistrict *findOwnable(const std::string &name);

private:
    std::vector<std::unique_ptr<District>> districts_;
};

#endif  // CS247_BOARD_H
