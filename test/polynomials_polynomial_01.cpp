// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <mp++/integer.hpp>

#include <obake/math/degree.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/symbols.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("polynomial_mul_simple_test_truncated")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z] = make_polynomials<poly_t>(symbol_set{"x", "y", "z"}, "x", "y", "z");
    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z"});

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, 100);
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, 2);
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, mppp::integer<1>{1});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, 0);
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, x + y, x - y, -1);
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 100);
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 3);
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, mppp::integer<1>{2});
    REQUIRE(retval == -z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 1);
    REQUIRE(retval == -y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, 0);
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, z * x + y, x - y - 1, -1);
    REQUIRE(retval.empty());
    retval.clear_terms();
}

TEST_CASE("polynomial_mul_simple_test_truncated_large")
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

    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z", "t", "u"});

    polynomials::detail::poly_mul_impl_simple(retval, f, g, 1000);
    REQUIRE(retval == cmp);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, f, g, 80);
    REQUIRE(retval == cmp);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, f, g, 40);
    REQUIRE(degree(retval) == 40);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, f, g, 5);
    REQUIRE(degree(retval) == 5);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, f, g, 0);
    REQUIRE(retval == 1);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_simple(retval, f, g, -1);
    REQUIRE(retval.empty());
    retval.clear_terms();
}

TEST_CASE("polynomial_mul_mt_hm_test_truncated")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z] = make_polynomials<poly_t>(symbol_set{"x", "y", "z"}, "x", "y", "z");
    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z"});

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, 100);
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, 2);
    REQUIRE(retval == x * x - y * y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, mppp::integer<1>{1});
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, 0);
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, x + y, x - y, -1);
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 100);
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 3);
    REQUIRE(retval == x * x * z - x * y * z - z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, mppp::integer<1>{2});
    REQUIRE(retval == -z * x + x * y - y * y - y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 1);
    REQUIRE(retval == -y);
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, 0);
    REQUIRE(retval.empty());
    retval.clear_terms();

    polynomials::detail::poly_mul_impl_mt_hm(retval, z * x + y, x - y - 1, -1);
    REQUIRE(retval.empty());
    retval.clear_terms();
}

TEST_CASE("polynomial_mul_mt_hm_test_truncated_large")
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

    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z", "t", "u"});

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, 1000);
    REQUIRE(retval == cmp);
    retval.clear_terms();
    retval.set_n_segments(0);

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, 80);
    REQUIRE(retval == cmp);
    retval.clear_terms();
    retval.set_n_segments(0);

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, 40);
    REQUIRE(degree(retval) == 40);
    retval.clear_terms();
    retval.set_n_segments(0);

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, 5);
    REQUIRE(degree(retval) == 5);
    retval.clear_terms();
    retval.set_n_segments(0);

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, 0);
    REQUIRE(retval == 1);
    retval.clear_terms();
    retval.set_n_segments(0);

    polynomials::detail::poly_mul_impl_mt_hm(retval, f, g, -1);
    REQUIRE(retval.empty());
    retval.clear_terms();
    retval.set_n_segments(0);
}
