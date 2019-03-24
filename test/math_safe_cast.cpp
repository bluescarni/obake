// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/math/safe_cast.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <string>

#include <piranha/config.hpp>

using namespace piranha;

// Make sure we can go constexpr, when the types and values allow.
constexpr auto cint = piranha::safe_cast<int>(5u);

TEST_CASE("safe_cast_test")
{
    REQUIRE(!is_safely_castable_v<void, void>);
    REQUIRE(!is_safely_castable_v<int, void>);
    REQUIRE(!is_safely_castable_v<void, int>);
    REQUIRE(is_safely_castable_v<int, int>);
    REQUIRE(is_safely_castable_v<long, int>);
    REQUIRE(is_safely_castable_v<long &, int>);
    REQUIRE(is_safely_castable_v<const long, int>);
    REQUIRE(is_safely_castable_v<const long &, int>);
    REQUIRE(is_safely_castable_v<long &&, int>);
    REQUIRE(is_safely_castable_v<double, double>);
    REQUIRE(is_safely_castable_v<std::string, std::string>);
    REQUIRE(!is_safely_castable_v<double, int>);
    REQUIRE(!is_safely_castable_v<long, int &>);
    REQUIRE(!is_safely_castable_v<long, const int>);
    REQUIRE(!is_safely_castable_v<long, const int &>);
    REQUIRE(!is_safely_castable_v<long, int &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!SafelyCastable<int, void>);
    REQUIRE(!SafelyCastable<void, int>);
    REQUIRE(SafelyCastable<int, int>);
    REQUIRE(SafelyCastable<long, int>);
    REQUIRE(SafelyCastable<long &, int>);
    REQUIRE(SafelyCastable<const long, int>);
    REQUIRE(SafelyCastable<const long &, int>);
    REQUIRE(SafelyCastable<long &&, int>);
    REQUIRE(SafelyCastable<double, double>);
    REQUIRE(SafelyCastable<std::string, std::string>);
    REQUIRE(!SafelyCastable<double, int>);
    REQUIRE(!SafelyCastable<long, int &>);
    REQUIRE(!SafelyCastable<long, const int>);
    REQUIRE(!SafelyCastable<long, const int &>);
    REQUIRE(!SafelyCastable<long, int &&>);
#endif

    //piranha::safe_cast<unsigned>(-5);
    // piranha::safe_cast<double>(5u);
}
