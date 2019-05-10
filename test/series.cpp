// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/series.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <type_traits>

#include <piranha/math/pow.hpp>

TEST_CASE("pow_test")
{
    using series_t = piranha::series<double, int, void>;

    series_t s;
    REQUIRE(s.empty());
    REQUIRE(s.begin() == s.end());
    REQUIRE(s.cbegin() == s.cend());

    REQUIRE(s.cbegin() == s.begin());

    series_t::const_iterator it0(s.begin());
    // series_t::iterator it1(s.cbegin());

    REQUIRE(std::is_nothrow_swappable_v<series_t>);
    REQUIRE(std::is_nothrow_swappable_v<series_t::const_iterator>);
    REQUIRE(std::is_nothrow_swappable_v<series_t::iterator>);
}
