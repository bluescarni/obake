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

#include <bitset>
#include <cstddef>
#include <iostream>
#include <limits>
#include <type_traits>

#include <piranha/hash.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/polynomials/packed_monomial.hpp>

using namespace piranha;

TEST_CASE("pow_test")
{
    using pm_t = packed_monomial<int>;
    using series_t = series<double, pm_t, void>;

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

    constexpr auto width = std::numeric_limits<std::size_t>::digits;

    std::cout << std::bitset<width>(detail::key_hasher{}(pm_t{1, 2, 3})) << " vs "
              << std::bitset<width>(hash(pm_t{1, 2, 3})) << '\n';
    std::cout << std::bitset<width>(detail::key_hasher{}(pm_t{4, 5, 6})) << " vs "
              << std::bitset<width>(hash(pm_t{4, 5, 6})) << '\n';
}
