// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>

#include <obake/math/degree.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/power_series/power_series.hpp>
#include <obake/symbols.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

TEST_CASE("basic")
{
    obake_test::disable_slow_stack_traces();

    // Type traits/concepts.
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    REQUIRE(power_series_cf<double>);
    REQUIRE(!power_series_cf<const double>);
    REQUIRE(!power_series_cf<const double &>);
    REQUIRE(!power_series_cf<void>);

    REQUIRE(power_series_key<pm_t>);
    REQUIRE(!power_series_key<const pm_t>);
    REQUIRE(!power_series_key<const pm_t &>);
    REQUIRE(!power_series_key<void>);

    REQUIRE(any_p_series<ps_t>);
    REQUIRE(!any_p_series<const ps_t>);
    REQUIRE(!any_p_series<ps_t &>);
    REQUIRE(!any_p_series<void>);

    // Default construction of the tag.
    ps_t foo;
    REQUIRE(foo.tag().trunc.get().index() == 0u);

    // Tag state after truncation setting.
    set_truncation(foo, 5);
    REQUIRE(foo.tag().trunc.get().index() == 1u);
    REQUIRE(std::is_reference_v<decltype(set_truncation(foo, 5))>);
    set_truncation(foo, 5, symbol_set{"x", "y", "z"});
    REQUIRE(foo.tag().trunc.get().index() == 2u);
    REQUIRE(std::is_reference_v<decltype(set_truncation(foo, 5, symbol_set{"x", "y", "z"}))>);
    unset_truncation(foo);
    REQUIRE(foo.tag().trunc.get().index() == 0u);
    REQUIRE(std::is_reference_v<decltype(unset_truncation(foo))>);

    // Truncation getter.
    REQUIRE(get_truncation(foo).index() == 0u);
    REQUIRE(std::is_reference_v<decltype(get_truncation(foo))>);

    // Factory functions.
    {
        // No truncation, no ss.
        auto [x, y] = make_p_series<ps_t>("x", std::string{"y"});

        REQUIRE(x.size() == 1u);
        REQUIRE(x.begin()->first == pm_t{1});
        REQUIRE(x.begin()->second == 1.);
        REQUIRE(x.get_symbol_set() == symbol_set{"x"});
        REQUIRE(get_truncation(x).index() == 0u);

        REQUIRE(y.size() == 1u);
        REQUIRE(y.begin()->first == pm_t{1});
        REQUIRE(y.begin()->second == 1.);
        REQUIRE(y.get_symbol_set() == symbol_set{"y"});
        REQUIRE(get_truncation(y).index() == 0u);
    }

    {
        // No truncation, with ss.
        auto [x, z] = make_p_series<ps_t>(symbol_set{"x", "y", "z"}, std::string{"x"}, "z");

        REQUIRE(x.size() == 1u);
        REQUIRE(x.begin()->first == pm_t{1, 0, 0});
        REQUIRE(x.begin()->second == 1.);
        REQUIRE(x.get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(get_truncation(x).index() == 0u);

        REQUIRE(z.size() == 1u);
        REQUIRE(z.begin()->first == pm_t{0, 0, 1});
        REQUIRE(z.begin()->second == 1.);
        REQUIRE(z.get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(get_truncation(z).index() == 0u);

        OBAKE_REQUIRES_THROWS_CONTAINS((make_p_series<ps_t>(symbol_set{"x", "y", "z"}, std::string{"x"}, "a")),
                                       std::invalid_argument,
                                       "Cannot create a power series with symbol set {'x', 'y', 'z'} from the "
                                       "generator 'a': the generator is not in the symbol set");
    }

    {
        // Total truncation, no ss.
        auto [x, y] = make_p_series_t<ps_t>(1, "x", std::string{"y"});

        REQUIRE(x.size() == 1u);
        REQUIRE(x.begin()->first == pm_t{1});
        REQUIRE(x.begin()->second == 1.);
        REQUIRE(x.get_symbol_set() == symbol_set{"x"});
        REQUIRE(get_truncation(x).index() == 1u);
        REQUIRE(std::get<1>(get_truncation(x)) == 1);

        REQUIRE(y.size() == 1u);
        REQUIRE(y.begin()->first == pm_t{1});
        REQUIRE(y.begin()->second == 1.);
        REQUIRE(y.get_symbol_set() == symbol_set{"y"});
        REQUIRE(get_truncation(y).index() == 1u);
        REQUIRE(std::get<1>(get_truncation(y)) == 1);

        auto [a, b] = make_p_series_t<ps_t>(0, "a", std::string{"b"});

        REQUIRE(a.empty());
        REQUIRE(a.get_symbol_set() == symbol_set{"a"});
        REQUIRE(std::get<1>(get_truncation(a)) == 0);

        REQUIRE(b.empty());
        REQUIRE(b.get_symbol_set() == symbol_set{"b"});
        REQUIRE(std::get<1>(get_truncation(b)) == 0);
    }
}
