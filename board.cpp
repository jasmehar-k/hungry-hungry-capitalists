#include "board.h"

#include <cassert>

#include "district.h"
#include "districtfactory.h"

Board::Board() {
    districts_.reserve(kBoard.size());
    for (const DistrictSpec &spec : kBoard) {
        districts_.push_back(DistrictFactory::create(spec));
    }
}

Board::~Board() = default;

const District &Board::at(int square) const {
    assert(square >= 0 && square < kBoardSize);
    return *districts_[static_cast<std::size_t>(square)];
}

District &Board::at(int square) {
    assert(square >= 0 && square < kBoardSize);
    return *districts_[static_cast<std::size_t>(square)];
}

int Board::advance(int from, int steps) {
    // Works for the backward moves a Wandering Market can order, too.
    const int raw = (from + steps) % kBoardSize;
    return raw < 0 ? raw + kBoardSize : raw;
}

bool Board::passedGrandEntrance(int from, int to) {
    return to < from;  // the circuit only wraps at Grand Entrance
}

OwnableDistrict *Board::findOwnable(const std::string &name) {
    for (const auto &district : districts_) {
        if (district->isOwnable() && district->name() == name) {
            return static_cast<OwnableDistrict *>(district.get());
        }
    }
    return nullptr;
}
