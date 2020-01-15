// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>

#include <obake/config.hpp>
#include <obake/math/safe_cast.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

// NOTE: don't run the test if constexpr
// support is not recent enough.
#if __cpp_constexpr >= 201603

// Make sure we can go constexpr, when the types and values allow.
[[maybe_unused]] constexpr auto cint = obake::safe_cast<int>(5u);

#endif

TEST_CASE("safe_cast_test")
{
    obake_test::disable_slow_stack_traces();

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

#if defined(OBAKE_HAVE_CONCEPTS)
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

    REQUIRE(obake::safe_cast<int>(5u) == 5);
    OBAKE_REQUIRES_THROWS_CONTAINS(obake::safe_cast<unsigned>(-5), safe_cast_failure, "A value of type '");
    OBAKE_REQUIRES_THROWS_CONTAINS(obake::safe_cast<unsigned>(-5), safe_cast_failure,
                                   "' could not be safely converted to the type '");
}
