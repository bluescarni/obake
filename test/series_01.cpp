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

#include <initializer_list>
#include <stdexcept>

#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/symbols.hpp>

#include <mp++/rational.hpp>

#include "test_utils.hpp"

using rat_t = mppp::rational<1>;

using namespace piranha;

TEST_CASE("series_is_single_cf")
{
    piranha_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    REQUIRE(s1_t{}.is_single_cf());
    REQUIRE(s1_t{"3/4"}.is_single_cf());
    s1_t s1;
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, "3/4");
    REQUIRE(!s1.is_single_cf());
}

TEST_CASE("series_set_symbol_set")
{
    using Catch::Matchers::Contains;

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    REQUIRE(s1.get_symbol_set() == symbol_set{"x", "y", "z"});

    s1 = s1_t{"3/4"};
    REQUIRE_THROWS_WITH(s1.set_symbol_set(symbol_set{}),
                        Contains("A symbol set can be set only in an empty series, but this series has 1 terms"));
    REQUIRE_THROWS_AS(s1.set_symbol_set(symbol_set{}), std::invalid_argument);
}

TEST_CASE("series_reserve")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    s1.reserve(42);
    REQUIRE(s1._get_s_table().size() == 1u);
    REQUIRE(s1._get_s_table()[0].bucket_count() != 0u);

    s1 = s1_t{};
    s1.set_n_segments(2);
    s1.reserve(32);
    REQUIRE(s1._get_s_table().size() == 4u);
    REQUIRE(s1._get_s_table()[0].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[1].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[2].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[3].bucket_count() != 0u);

    s1 = s1_t{};
    s1.set_n_segments(2);
    s1.reserve(37);
    REQUIRE(s1._get_s_table().size() == 4u);
    REQUIRE(s1._get_s_table()[0].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[1].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[2].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[3].bucket_count() != 0u);
}

TEST_CASE("series_set_n_segments")
{
    using Catch::Matchers::Contains;

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    s1.set_n_segments(0);
    REQUIRE(s1._get_s_table().size() == 1u);
    s1.set_n_segments(1);
    REQUIRE(s1._get_s_table().size() == 2u);
    s1.set_n_segments(2);
    REQUIRE(s1._get_s_table().size() == 4u);
    s1.set_n_segments(4);
    REQUIRE(s1._get_s_table().size() == 16u);
    REQUIRE_THROWS_WITH(s1.set_n_segments(static_cast<unsigned>(s1._get_max_log2_size() + 1u)),
                        Contains(" as this value exceeds the maximum allowed value"));
    REQUIRE_THROWS_AS(s1.set_n_segments(static_cast<unsigned>(s1._get_max_log2_size() + 1u)), std::invalid_argument);
}

TEST_CASE("series_clear")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    s1.set_n_segments(2);
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, 1);
    s1.add_term(pm_t{-1, -2, -3}, -1);
    s1.add_term(pm_t{4, 5, 6}, 2);
    s1.add_term(pm_t{7, 8, 9}, -2);
    s1.clear();

    REQUIRE(s1.empty());
    REQUIRE(s1.get_symbol_set() == symbol_set{});
}
