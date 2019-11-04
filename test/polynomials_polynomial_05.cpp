// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <initializer_list>

#include <mp++/integer.hpp>

#include <obake/math/truncate_p_degree.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/symbols.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("polynomial_truncate_p_degree")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;
    using ppoly_t = polynomial<pm_t, poly_t>;

    REQUIRE(!is_p_degree_truncatable_v<poly_t, void>);
    REQUIRE(is_p_degree_truncatable_v<poly_t, int>);
    REQUIRE(is_p_degree_truncatable_v<poly_t, int &>);
    REQUIRE(is_p_degree_truncatable_v<poly_t &, int &>);
    REQUIRE(is_p_degree_truncatable_v<const poly_t &, const int>);
    REQUIRE(is_p_degree_truncatable_v<const poly_t, const int &>);
    // Mixed cf/key truncation not supported at this time.
    REQUIRE(!is_p_degree_truncatable_v<ppoly_t, int>);

    auto [x, y, z] = make_polynomials<poly_t>("x", "y", "z");

    const auto p = x * y * z - 3 * x + 4 * x * y - z + 5;

    REQUIRE(truncate_p_degree(p, 100, symbol_set{"x", "y", "z"}) == p);
    REQUIRE(truncate_p_degree(p, 100, symbol_set{"x", "y"}) == p);
    REQUIRE(truncate_p_degree(p, 100, symbol_set{"x"}) == p);
    REQUIRE(truncate_p_degree(p, 100, symbol_set{"y", "z"}) == p);
    REQUIRE(truncate_p_degree(p, 100, symbol_set{}) == p);

    REQUIRE(truncate_p_degree(p, 3, symbol_set{"x", "y", "z"}) == p);
    REQUIRE(truncate_p_degree(p, 3, symbol_set{"x"}) == p);
    REQUIRE(truncate_p_degree(p, 3, symbol_set{"x", "y"}) == p);
    REQUIRE(truncate_p_degree(p, 3, symbol_set{"z", "y"}) == p);
    REQUIRE(truncate_p_degree(p, 3, symbol_set{}) == p);

    REQUIRE(truncate_p_degree(p, 2, symbol_set{"x", "y", "z"}) == -3 * x + 4 * x * y - z + 5);
    REQUIRE(truncate_p_degree(p, 2, symbol_set{"x", "y"}) == p);
    REQUIRE(truncate_p_degree(p, 2, symbol_set{"x", "z"}) == p);
    REQUIRE(truncate_p_degree(p, 2, symbol_set{"x"}) == p);
    REQUIRE(truncate_p_degree(p, 2, symbol_set{"z"}) == p);
    REQUIRE(truncate_p_degree(p, 2, symbol_set{}) == p);

    REQUIRE(truncate_p_degree(p, 1, symbol_set{"x", "y", "z"}) == -3 * x - z + 5);
    REQUIRE(truncate_p_degree(p, 1, symbol_set{"x", "y"}) == -3 * x - z + 5);
    REQUIRE(truncate_p_degree(p, 1, symbol_set{"x"}) == p);
    REQUIRE(truncate_p_degree(p, 1, symbol_set{"y"}) == p);
    REQUIRE(truncate_p_degree(p, 1, symbol_set{}) == p);

    REQUIRE(truncate_p_degree(p, 0, symbol_set{"x", "y", "z"}) == 5);
    REQUIRE(truncate_p_degree(p, 0, symbol_set{"x", "y"}) == -z + 5);
    REQUIRE(truncate_p_degree(p, 0, symbol_set{"x"}) == -z + 5);
    REQUIRE(truncate_p_degree(p, 0, symbol_set{}) == p);

    REQUIRE(truncate_p_degree(p, -1, symbol_set{"x", "y", "z"}).empty());
    REQUIRE(truncate_p_degree(p, -1, symbol_set{"x"}).empty());
    REQUIRE(truncate_p_degree(p, -1, symbol_set{"y"}).empty());
    REQUIRE(truncate_p_degree(p, -1, symbol_set{}).empty());

    REQUIRE(truncate_p_degree(p, -100, symbol_set{"x", "y", "z"}).empty());
    REQUIRE(truncate_p_degree(p, -100, symbol_set{"x", "y"}).empty());
    REQUIRE(truncate_p_degree(p, -100, symbol_set{"x"}).empty());
    REQUIRE(truncate_p_degree(p, -100, symbol_set{}).empty());
}

// Exercise the segmented tables layout.
TEST_CASE("polynomial_truncate_p_degree_large")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z, t, u] = make_polynomials<poly_t>("x", "y", "z", "t", "u");

    auto f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
    const auto tmp_f(f);
    auto g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
    const auto tmp_g(g);

    for (int i = 1; i < 8; ++i) {
        f *= tmp_f;
        g *= tmp_g;
    }

    const auto cmp = f * g;
    const auto tcmp = truncated_mul(f, g, 50, symbol_set{"x", "z", "u"});

    REQUIRE(truncate_p_degree(cmp, 50, symbol_set{"x", "z", "u"}) == tcmp);
}
