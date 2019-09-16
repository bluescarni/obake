// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>

#include <piranha/config.hpp>
#include <piranha/math/safe_cast.hpp>

#include "catch.hpp"

using namespace piranha;

#if __cpp_constexpr >= 201603

// Make sure we can go constexpr, when the types and values allow.
[[maybe_unused]] constexpr auto cint = piranha::safe_cast<int>(5u);

#endif

TEST_CASE("safe_cast_test")
{
    using Catch::Matchers::Contains;

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

    REQUIRE(piranha::safe_cast<int>(5u) == 5);
    REQUIRE_THROWS_WITH(piranha::safe_cast<unsigned>(-5), Contains("A value of type '"));
    REQUIRE_THROWS_WITH(piranha::safe_cast<unsigned>(-5), Contains("' could not be safely converted to the type '"));
}
