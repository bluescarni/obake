// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <cstdint>
// #include <initializer_list>
// #include <sstream>
#include <stdexcept>
// #include <string>
// #include <tuple>
#include <type_traits>
// #include <utility>
// #include <variant>

// #include <boost/algorithm/string/predicate.hpp>
// #include <boost/archive/binary_iarchive.hpp>
// #include <boost/archive/binary_oarchive.hpp>

// #include <obake/hash.hpp>
// #include <obake/math/truncate_degree.hpp>
// #include <obake/math/truncate_p_degree.hpp>
#include <obake/polynomials/packed_monomial.hpp>
// #include <obake/polynomials/polynomial.hpp>
#include <obake/power_series/power_series.hpp>
#include <obake/symbols.hpp>
// #include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

TEST_CASE("in place add")
{
    obake_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    {
        auto [x] = make_p_series<ps_t>("x");

        // Check that the primitive returns a reference.
        REQUIRE(std::is_reference_v<decltype(power_series::series_in_place_add(x, 1))>);
        REQUIRE(std::is_reference_v<decltype(x += 2.)>);

        x += 2.;
        REQUIRE(x.size() == 2u);
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.second == 1.; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.second == 2.; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.first == pm_t{1}; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.first == pm_t{0}; }));
    }

    // Example with truncation.
    {
        auto [x] = make_p_series_t<ps_t>(-1, "x");

        REQUIRE(x.empty());
        x += 2.;
        REQUIRE(x.empty());
    }

    // Same-rank series.
    {
        auto [x, y] = make_p_series_t<ps_t>(10, "x", "y");

        // Check that the primitive returns a reference.
        REQUIRE(std::is_reference_v<decltype(power_series::series_in_place_add(x, y))>);
        REQUIRE(std::is_reference_v<decltype(x += y)>);

        x += y;
        REQUIRE(x.size() == 2u);
        REQUIRE(x.get_symbol_set() == symbol_set{"x", "y"});
        REQUIRE(std::all_of(x.begin(), x.end(), [](const auto &t) { return t.second == 1.; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.first == pm_t{1, 0}; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.first == pm_t{0, 1}; }));
    }

    // Check incompatible truncation levels.
    {
        auto [x] = make_p_series_t<ps_t>(10, "x");
        auto [y] = make_p_series_t<ps_t>(20, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x += y, std::invalid_argument,
                                       "Unable to add two power series in place if "
                                       "their truncation levels do not match");
    }
    {
        auto [x] = make_p_series<ps_t>("x");
        auto [y] = make_p_series_t<ps_t>(20, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x += y, std::invalid_argument,
                                       "Unable to add two power series in place if "
                                       "their truncation levels do not match");
    }
    {
        auto [x] = make_p_series_p<ps_t>(10, symbol_set{"a"}, "x");
        auto [y] = make_p_series_t<ps_t>(20, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x += y, std::invalid_argument,
                                       "Unable to add two power series in place if "
                                       "their truncation levels do not match");
    }
}

TEST_CASE("in place sub")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    {
        auto [x] = make_p_series<ps_t>("x");

        // Check that the primitive returns a reference.
        REQUIRE(std::is_reference_v<decltype(power_series::series_in_place_sub(x, 1))>);
        REQUIRE(std::is_reference_v<decltype(x -= 2.)>);

        x -= 2.;
        REQUIRE(x.size() == 2u);
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.second == 1.; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.second == -2.; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.first == pm_t{1}; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.first == pm_t{0}; }));
    }

    // Example with truncation.
    {
        auto [x] = make_p_series_t<ps_t>(-1, "x");

        REQUIRE(x.empty());
        x -= 2.;
        REQUIRE(x.empty());
    }

    // Same-rank series.
    {
        auto [x, y] = make_p_series_t<ps_t>(10, "x", "y");

        // Check that the primitive returns a reference.
        REQUIRE(std::is_reference_v<decltype(power_series::series_in_place_sub(x, y))>);
        REQUIRE(std::is_reference_v<decltype(x -= y)>);

        x -= y;
        REQUIRE(x.size() == 2u);
        REQUIRE(x.get_symbol_set() == symbol_set{"x", "y"});
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.second == 1.; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.second == -1.; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.first == pm_t{1, 0}; }));
        REQUIRE(std::any_of(x.begin(), x.end(), [](const auto &t) { return t.first == pm_t{0, 1}; }));
    }

    // Check incompatible truncation levels.
    {
        auto [x] = make_p_series_t<ps_t>(10, "x");
        auto [y] = make_p_series_t<ps_t>(20, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x -= y, std::invalid_argument,
                                       "Unable to subtract two power series in place if "
                                       "their truncation levels do not match");
    }
    {
        auto [x] = make_p_series<ps_t>("x");
        auto [y] = make_p_series_t<ps_t>(20, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x -= y, std::invalid_argument,
                                       "Unable to subtract two power series in place if "
                                       "their truncation levels do not match");
    }
    {
        auto [x] = make_p_series_p<ps_t>(10, symbol_set{"a"}, "x");
        auto [y] = make_p_series_t<ps_t>(20, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x -= y, std::invalid_argument,
                                       "Unable to subtract two power series in place if "
                                       "their truncation levels do not match");
    }
}
