// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/math/is_zero.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <piranha/config.hpp>

struct no_is_zero {
};

TEST_CASE("is_zero_default")
{
    // Check type-traits/concepts.
    REQUIRE(piranha::is_zero_testable_v<float>);
    REQUIRE(piranha::is_zero_testable_v<int>);
    REQUIRE(piranha::is_zero_testable_v<double &&>);
    REQUIRE(piranha::is_zero_testable_v<const long double>);
    REQUIRE(piranha::is_zero_testable_v<short &>);
    REQUIRE(piranha::is_zero_testable_v<const char &>);
    REQUIRE(!piranha::is_zero_testable_v<no_is_zero>);
    REQUIRE(!piranha::is_zero_testable_v<void>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::is_zero_testable_v<__int128_t>);
    REQUIRE(piranha::is_zero_testable_v<__uint128_t>);
    REQUIRE(piranha::is_zero_testable_v<__int128_t &&>);
    REQUIRE(piranha::is_zero_testable_v<const __uint128_t>);
    REQUIRE(piranha::is_zero_testable_v<const __uint128_t &>);
    REQUIRE(piranha::is_zero_testable_v<__int128_t &>);
#endif
#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::ZeroTestable<float>);
    REQUIRE(piranha::ZeroTestable<int>);
    REQUIRE(piranha::ZeroTestable<double &&>);
    REQUIRE(piranha::ZeroTestable<const long double>);
    REQUIRE(piranha::ZeroTestable<short &>);
    REQUIRE(piranha::ZeroTestable<const char &>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::ZeroTestable<__int128_t>);
    REQUIRE(piranha::ZeroTestable<__uint128_t>);
    REQUIRE(piranha::ZeroTestable<__int128_t &&>);
    REQUIRE(piranha::ZeroTestable<const __uint128_t>);
    REQUIRE(piranha::ZeroTestable<const __uint128_t &>);
    REQUIRE(piranha::ZeroTestable<__int128_t &>);
#endif
#endif
}
