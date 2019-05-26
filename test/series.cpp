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
#include <initializer_list>
#include <iostream>
#include <limits>
#include <type_traits>
#include <utility>

#include <mp++/rational.hpp>

#include <piranha/hash.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

template <typename T, typename U>
using series_add_t = decltype(series_add(std::declval<T>(), std::declval<U>()));

TEST_CASE("pow_test")
{
    using rat_t = mppp::rational<1>;

    using pm_t = packed_monomial<int>;
    using series_t = series<pm_t, double, void>;
    using series_t_rat = series<pm_t, rat_t, void>;

    REQUIRE(series_rank<void> == 0u);
    REQUIRE(series_rank<series_t> == 1u);
    REQUIRE(series_rank<series_t &> == 0u);

    REQUIRE(!is_detected_v<series_add_t, int, int>);

    REQUIRE(detail::series_add_strategy<int &, int &> == 0);
    REQUIRE(detail::series_add_strategy<int &, series_t &> == 1);
    REQUIRE(detail::series_add_strategy<series_t &, int &> == 2);

    series_t s, s2(4);
    s.set_nsegments(4);
    std::cout << s2 + 3.5 << '\n';

    series_t_rat sa(s2 + 3.5);
    std::cout << sa << '\n';

    std::cout << s << '\n';
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

    s.set_symbol_set({"x", "y", "z"});
    s.add_term(pm_t{0, 1, 2}, -1.);
    s.add_term(pm_t{2, 4, 5}, -2.);
    s.add_term(pm_t{2, 4, 5}, -5.);
    s.add_term(pm_t{0, 0, 0}, -1.);
    s.add_term(pm_t{0, 0, 0}, 1.);
    s.add_term(pm_t{0, 0, 0}, -1.);

    s.add_term<false>(pm_t{2, 0, -1}, -1.3);
    s.add_term(pm_t{2, 1, -1}, -1.);
    s.add_term<false>(pm_t{2, 1, -1}, -1.);

    std::cout << s << '\n';
}
