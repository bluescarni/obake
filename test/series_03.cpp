// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stdexcept>
#include <type_traits>

#include <mp++/rational.hpp>

#include <obake/byte_size.hpp>
#include <obake/detail/limits.hpp>
#include <obake/math/degree.hpp>
#include <obake/math/p_degree.hpp>
#include <obake/math/pow.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using rat_t = mppp::rational<1>;

using namespace obake;

TEST_CASE("series_byte_size")
{
    obake_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    s1.set_symbol_set(symbol_set{"x", "y"});
    s1.add_term(pm_t{1, 1}, 1);
    s1.add_term(pm_t{2, 2}, 2);

    REQUIRE(byte_size(s1) > sizeof(s1_t));

    // s2 has more terms than s1.
    s1_t s2;
    s2.set_symbol_set(symbol_set{"x", "y"});
    s2.add_term(pm_t{1, 1}, 1);
    s2.add_term(pm_t{2, 2}, 2);
    s2.add_term(pm_t{3, 3}, 3);

    REQUIRE(byte_size(s2) >= byte_size(s1));

    // s3 has more symbols than s2.
    s1_t s3;
    s3.set_symbol_set(symbol_set{"x", "y", "z"});
    s3.add_term(pm_t{1, 1, 1}, 1);
    s3.add_term(pm_t{2, 2, 2}, 2);
    s3.add_term(pm_t{3, 3, 3}, 3);

    REQUIRE(byte_size(s3) >= byte_size(s2));
}

TEST_CASE("series_degree")
{
    using pm_t = packed_monomial<int>;
    using s1_t = polynomial<pm_t, rat_t>;
    using s11_t = polynomial<pm_t, s1_t>;

    REQUIRE(is_with_degree_v<s1_t>);
    REQUIRE(is_with_degree_v<const s1_t>);
    REQUIRE(is_with_degree_v<s1_t &>);
    REQUIRE(is_with_degree_v<const s1_t &>);
    REQUIRE(is_with_degree_v<s1_t &&>);

    REQUIRE(is_with_degree_v<s11_t>);
    REQUIRE(is_with_degree_v<const s11_t>);
    REQUIRE(is_with_degree_v<s11_t &>);
    REQUIRE(is_with_degree_v<const s11_t &>);
    REQUIRE(is_with_degree_v<s11_t &&>);

    {
        REQUIRE(degree(s1_t{}) == 0);

        auto [x, y, z] = make_polynomials<s1_t>("x", "y", "z");
        REQUIRE(std::is_same_v<decltype(degree(x)), int>);
        REQUIRE(degree(x * 0 + 1) == 0);
        REQUIRE(degree(x) == 1);
        REQUIRE(degree(y) == 1);
        REQUIRE(degree(z) == 1);

        REQUIRE(degree(x * x) == 2);
        REQUIRE(degree(y * x) == 2);
        REQUIRE(degree(z * z) == 2);
        REQUIRE(degree((x + y) * (x - y)) == 2);
        REQUIRE(degree((x + y) * (x - y) - z) == 2);
        REQUIRE(degree((x + y) * (x - y) - x * z * y) == 3);
        REQUIRE(degree((x + y) * (x - y) - x * z * y + 1) == 3);
    }

    {
        REQUIRE(degree(s11_t{}) == 0);

        auto [y, z] = make_polynomials<s11_t>("y", "z");
        auto [x] = make_polynomials<s1_t>("x");
        REQUIRE(std::is_same_v<decltype(degree(x)), int>);
        REQUIRE(std::is_same_v<decltype(degree(y)), int>);
        REQUIRE(std::is_same_v<decltype(degree(x * y)), int>);
        REQUIRE(degree(x * 0 + 1) == 0);
        REQUIRE(degree(x) == 1);
        REQUIRE(degree(y * 0 + 1) == 0);
        REQUIRE(degree(y) == 1);
        REQUIRE(degree(z) == 1);

        REQUIRE(degree(x * x) == 2);
        REQUIRE(degree(y * x) == 2);
        REQUIRE(degree(z * z) == 2);
        REQUIRE(degree((x + y) * (x - y)) == 2);
        REQUIRE(degree((x + y) * (x - y) - z) == 2);
        REQUIRE(degree((x + y) * (x - y) - x * z * y) == 3);
        REQUIRE(degree((x + y) * (x - y) - x * z * y + 1) == 3);
    }

#if !defined(_MSC_VER) || defined(__clang__)
    {
        // Overflow checking.
        auto [x] = make_polynomials<s1_t>("x");
        auto [y] = make_polynomials<s11_t>("y");

        x = obake::pow(x, detail::limits_max<int>);
        y = obake::pow(y, detail::limits_max<int>);

        OBAKE_REQUIRES_THROWS_CONTAINS(degree(x * y), std::overflow_error, "Overflow error in an integral ");
    }
#endif
}

TEST_CASE("series_p_degree")
{
    using pm_t = packed_monomial<int>;
    using s1_t = polynomial<pm_t, rat_t>;
    using s11_t = polynomial<pm_t, s1_t>;

    REQUIRE(is_with_p_degree_v<s1_t>);
    REQUIRE(is_with_p_degree_v<const s1_t>);
    REQUIRE(is_with_p_degree_v<s1_t &>);
    REQUIRE(is_with_p_degree_v<const s1_t &>);
    REQUIRE(is_with_p_degree_v<s1_t &&>);

    REQUIRE(is_with_p_degree_v<s11_t>);
    REQUIRE(is_with_p_degree_v<const s11_t>);
    REQUIRE(is_with_p_degree_v<s11_t &>);
    REQUIRE(is_with_p_degree_v<const s11_t &>);
    REQUIRE(is_with_p_degree_v<s11_t &&>);

    {
        REQUIRE(p_degree(s1_t{}, symbol_set{}) == 0);
        REQUIRE(p_degree(s1_t{}, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(s1_t{}, symbol_set{"x", "y", "z"}) == 0);

        auto [x, y, z] = make_polynomials<s1_t>("x", "y", "z");
        REQUIRE(std::is_same_v<decltype(p_degree(x, symbol_set{})), int>);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"x", "y", "z"}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"x", "z"}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"y", "z"}) == 0);
        REQUIRE(p_degree(x, symbol_set{}) == 0);
        REQUIRE(p_degree(x, symbol_set{"x"}) == 1);
        REQUIRE(p_degree(x, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(x, symbol_set{"z"}) == 0);
        REQUIRE(p_degree(x, symbol_set{"x", "y"}) == 1);
        REQUIRE(p_degree(x, symbol_set{"x", "z"}) == 1);
        REQUIRE(p_degree(x, symbol_set{"y", "z"}) == 0);
        REQUIRE(p_degree(y, symbol_set{"y"}) == 1);
        REQUIRE(p_degree(y, symbol_set{}) == 0);
        REQUIRE(p_degree(y, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(y, symbol_set{"x", "y"}) == 1);
        REQUIRE(p_degree(y, symbol_set{"x", "z"}) == 0);
        REQUIRE(p_degree(y, symbol_set{"y", "z"}) == 1);
        REQUIRE(p_degree(z, symbol_set{"z"}) == 1);
        REQUIRE(p_degree(z, symbol_set{}) == 0);
        REQUIRE(p_degree(z, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(z, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(z, symbol_set{"x", "y"}) == 0);
        REQUIRE(p_degree(z, symbol_set{"x", "z"}) == 1);
        REQUIRE(p_degree(z, symbol_set{"y", "z"}) == 1);

        REQUIRE(p_degree(x * x, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree(x * x, symbol_set{"x"}) == 2);
        REQUIRE(p_degree(x * x, symbol_set{}) == 0);
        REQUIRE(p_degree(x * x, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(y * x, symbol_set{"y"}) == 1);
        REQUIRE(p_degree(y * x, symbol_set{"y", "x"}) == 2);
        REQUIRE(p_degree(y * x, symbol_set{"x"}) == 1);
        REQUIRE(p_degree(y * x, symbol_set{"z"}) == 0);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"x"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"z"}) == 0);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x", "y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "y", "z"}) == 3);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 1, symbol_set{"x", "y", "z"}) == 3);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 1, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 2, symbol_set{"x", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 2, symbol_set{"y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 3, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 3, symbol_set{}) == 0);
    }

    {
        REQUIRE(p_degree(s11_t{}, symbol_set{}) == 0);
        REQUIRE(p_degree(s11_t{}, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(s11_t{}, symbol_set{"x", "y"}) == 0);

        auto [y, z] = make_polynomials<s11_t>("y", "z");
        auto [x] = make_polynomials<s1_t>("x");
        REQUIRE(std::is_same_v<decltype(p_degree(x, symbol_set{})), int>);
        REQUIRE(std::is_same_v<decltype(p_degree(y, symbol_set{})), int>);
        REQUIRE(std::is_same_v<decltype(p_degree(x * y, symbol_set{})), int>);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"y", "x"}) == 0);
        REQUIRE(p_degree(x, symbol_set{"x"}) == 1);
        REQUIRE(p_degree(x, symbol_set{"x", "y"}) == 1);
        REQUIRE(p_degree(x, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(x, symbol_set{}) == 0);
        REQUIRE(p_degree(y * 0 + 1, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(y * 0 + 1, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(y * 0 + 1, symbol_set{}) == 0);
        REQUIRE(p_degree(y, symbol_set{"y"}) == 1);
        REQUIRE(p_degree(y, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(y, symbol_set{"x", "y"}) == 1);
        REQUIRE(p_degree(y, symbol_set{}) == 0);
        REQUIRE(p_degree(z, symbol_set{"z"}) == 1);
        REQUIRE(p_degree(z, symbol_set{"z", "x"}) == 1);
        REQUIRE(p_degree(z, symbol_set{"z", "y"}) == 1);
        REQUIRE(p_degree(z, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(z, symbol_set{}) == 0);

        REQUIRE(p_degree(x * x, symbol_set{"x"}) == 2);
        REQUIRE(p_degree(x * x, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(x * x, symbol_set{"y", "x"}) == 2);
        REQUIRE(p_degree(x * x, symbol_set{}) == 0);
        REQUIRE(p_degree(y * x, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree(y * x, symbol_set{"x", "y", "z"}) == 2);
        REQUIRE(p_degree(y * x, symbol_set{"x"}) == 1);
        REQUIRE(p_degree(y * x, symbol_set{"y"}) == 1);
        REQUIRE(p_degree(y * x, symbol_set{"z"}) == 0);
        REQUIRE(p_degree(y * x, symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"x", "y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"x"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"z"}) == 0);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x", "y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "y", "z"}) == 3);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 1, symbol_set{"x", "y", "z"}) == 3);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 1, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 2, symbol_set{"x", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 2, symbol_set{"y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 3, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 3, symbol_set{"x"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 4, symbol_set{"y"}) == 2);
    }

#if !defined(_MSC_VER) || defined(__clang__)
    {
        // Overflow checking.
        auto [x] = make_polynomials<s1_t>("x");
        auto [y] = make_polynomials<s11_t>("y");

        x = obake::pow(x, detail::limits_max<int>);
        y = obake::pow(y, detail::limits_max<int>);

        OBAKE_REQUIRES_THROWS_CONTAINS(p_degree(x * y, symbol_set{"x", "y"}), std::overflow_error,
                                       "Overflow error in an integral ");
        REQUIRE(p_degree(x * y, symbol_set{"x"}) == detail::limits_max<int>);
        REQUIRE(p_degree(x * y, symbol_set{"y"}) == detail::limits_max<int>);
    }
#endif
}
