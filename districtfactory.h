#ifndef CS247_DISTRICTFACTORY_H
#define CS247_DISTRICTFACTORY_H

#include <memory>

#include "boarddata.h"

class District;

// This is the Simple Factory idiom, not the GoF Factory Method pattern. The set
// of district kinds is fixed by the spec and will never grow, which is exactly
// the case Ch 12 says Simple Factory suits. There is no creator hierarchy here
// and none is warranted, so the design report should name it accordingly.
class DistrictFactory {
public:
    static std::unique_ptr<District> create(const DistrictSpec &spec);
};

#endif  // CS247_DISTRICTFACTORY_H
