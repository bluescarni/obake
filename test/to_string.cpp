// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/detail/to_string.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <string>

#include <piranha/config.hpp>

using namespace piranha::detail;

TEST_CASE("to_string_test")
{
    REQUIRE(std::to_string(-45) == to_string(-45));
    REQUIRE(std::to_string(char(45)) == to_string(char(45)));
    REQUIRE(std::to_string(1.2345) == to_string(1.2345));

#if defined(PIRANHA_HAVE_GCC_INT128)
    // Zeroes.
    REQUIRE(to_string(__uint128_t(0)) == "0");
    REQUIRE(to_string(__int128_t(0)) == "0");

    // Small values.
    REQUIRE(to_string(__uint128_t(1)) == "1");
    REQUIRE(to_string(__int128_t(1)) == "1");
    REQUIRE(to_string(__int128_t(-1)) == "-1");
    REQUIRE(to_string(__uint128_t(123)) == "123");
    REQUIRE(to_string(__int128_t(123)) == "123");
    REQUIRE(to_string(__int128_t(-123)) == "-123");

    // Larger values.
    REQUIRE(to_string(__uint128_t(-1) / 100u) == "3402823669209384634633746074317682114");
    REQUIRE(to_string(static_cast<__int128_t>(__uint128_t(-1) / 100u)) == "3402823669209384634633746074317682114");
    REQUIRE(to_string(-static_cast<__int128_t>(__uint128_t(-1) / 100u)) == "-3402823669209384634633746074317682114");

    // Limit values.
    REQUIRE(to_string(__uint128_t(-1)) == "340282366920938463463374607431768211455");
    constexpr auto max_int128_t = (((__int128_t(1) << 126) - 1) << 1) + 1;
    REQUIRE(to_string(max_int128_t) == "170141183460469231731687303715884105727");
    REQUIRE(to_string(-max_int128_t - 1) == "-170141183460469231731687303715884105728");
#endif
}
