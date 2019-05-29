// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/cast.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <limits>
#include <type_traits>

#include <piranha/config.hpp>

using namespace piranha;

// Non-castable.
struct noncast00 {
};

namespace ns
{

struct cast00 {
};

template <typename T>
T cast(cast00);

struct noncast01 {
};

// Wrong return type.
template <typename T>
void cast(noncast01);

struct cast01 {
};

// Valid only on some overloads.
template <typename T>
T cast(cast01 &);

} // namespace ns

TEST_CASE("cast_test")
{
    REQUIRE(cast<int>(5.6) == 5);
    REQUIRE(std::is_same_v<int, decltype(cast<int>(5.6))>);
    REQUIRE(cast<double>(-1) == -1.);
    REQUIRE(std::is_same_v<double, decltype(cast<double>(-1))>);
    REQUIRE(cast<unsigned>(-1) == std::numeric_limits<unsigned>::max());
    REQUIRE(std::is_same_v<void, decltype(cast<void>(-1))>);

    REQUIRE(!is_castable_v<void, void>);
    REQUIRE(is_castable_v<int, void>);
    REQUIRE(!is_castable_v<void, int>);

    REQUIRE(is_castable_v<int, double>);
    REQUIRE(is_castable_v<int &, double>);
    REQUIRE(is_castable_v<const int &, double>);

    REQUIRE(is_castable_v<double, int>);
    REQUIRE(is_castable_v<double &, int>);
    REQUIRE(is_castable_v<const double &, int>);

    REQUIRE(!is_castable_v<int, double &>);
    REQUIRE(!is_castable_v<int &, const double>);
    REQUIRE(!is_castable_v<const int &, volatile double>);

    REQUIRE(!is_castable_v<int, noncast00>);
    REQUIRE(!is_castable_v<int &, noncast00>);
    REQUIRE(!is_castable_v<const int &, noncast00>);

    REQUIRE(is_castable_v<ns::cast00, noncast00>);
    REQUIRE(is_castable_v<ns::cast00 &, noncast00>);
    REQUIRE(is_castable_v<const ns::cast00 &&, noncast00>);

    REQUIRE(is_castable_v<ns::cast00, int>);
    REQUIRE(is_castable_v<ns::cast00 &, int>);
    REQUIRE(is_castable_v<const ns::cast00 &&, int>);

    REQUIRE(is_castable_v<ns::cast00, void>);
    REQUIRE(is_castable_v<ns::cast00 &, void>);
    REQUIRE(is_castable_v<const ns::cast00 &&, void>);

    REQUIRE(is_castable_v<ns::cast00, int &>);
    REQUIRE(!is_castable_v<ns::cast00 &, const int>);
    REQUIRE(!is_castable_v<const ns::cast00 &&, volatile int>);

    REQUIRE(!is_castable_v<ns::noncast01, int>);
    REQUIRE(!is_castable_v<ns::noncast01 &, int>);
    REQUIRE(!is_castable_v<const ns::noncast01 &&, int>);

    REQUIRE(!is_castable_v<ns::cast01, int>);
    REQUIRE(is_castable_v<ns::cast01 &, int>);
    REQUIRE(!is_castable_v<const ns::cast01 &, int>);
}
