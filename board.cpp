#include "board.h"

#include <cassert>

#include "district.h"
#include "districtfactory.h"
#include "workshop.h"

Board::Board() {
    districts_.reserve(kBoard.size());
    for (const DistrictSpec &spec : kBoard) {
        districts_.push_back(DistrictFactory::create(spec));
        // The one place districts are classified by concrete type. Everything
        // downstream iterates this list rather than casting board squares.
        if (auto *ownable = dynamic_cast<OwnableDistrict *>(districts_.back().get())) {
            ownables_.push_back(ownable);
        }
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

int Board::grandEntrancePasses(int from, int steps) {
    // Moving backward over Grand Entrance pays nothing, so a bare "did the
    // square index decrease" test would wrongly pay out on every backward move.
    if (steps <= 0) return 0;
    return (from + steps) / kBoardSize;
}

std::vector<const OwnableDistrict *> Board::ownables() const {
    return {ownables_.begin(), ownables_.end()};
}

OwnableDistrict *Board::findOwnable(const std::string &name) {
    for (OwnableDistrict *ownable : ownables_) {
        if (ownable->name() == name) return ownable;
    }
    return nullptr;
}

Workshop *Board::findWorkshop(const std::string &name) {
    OwnableDistrict *ownable = findOwnable(name);
    if (ownable == nullptr || ownable->kind() != DistrictKind::Workshop) return nullptr;
    // The kind check makes this downcast safe; it is the only one outside the
    // constructor's classification pass.
    return static_cast<Workshop *>(ownable);
}
