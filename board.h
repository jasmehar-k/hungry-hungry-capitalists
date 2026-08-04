#ifndef BOARD_H
#define BOARD_H

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

    // Owns its districts outright; copying would duplicate the circuit.
    Board(const Board &) = delete;
    Board &operator=(const Board &) = delete;

    static constexpr int size() { return kBoardSize; }

    const District &at(int square) const;
    District &at(int square);

    // Advance a position by a number of squares, wrapping the circuit.
    // Negative steps are supported: a Wandering Market can send an Artificer back.
    static int advance(int from, int steps);

    // How many times a move crosses Grand Entrance. This takes the step count
    // rather than the destination because the destination alone cannot tell a
    // forward lap from a backward move, and only forward moves collect. Real
    // dice never lap twice, but -testing accepts any non-negative values.
    static int grandEntrancePasses(int from, int steps);
    static bool passedGrandEntrance(int from, int steps) {
        return grandEntrancePasses(from, steps) > 0;
    }

    // Ownable district names are unique across the circuit, so this is the
    // lookup the save file and the district-naming commands need.
    OwnableDistrict *findOwnable(const std::string &name);

private:
    std::vector<std::unique_ptr<District>> districts_;
};

#endif  // BOARD_H
