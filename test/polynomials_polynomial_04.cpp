// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <initializer_list>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <obake/detail/tuple_for_each.hpp>
#include <obake/math/diff.hpp>
#include <obake/math/integrate.hpp>
#include <obake/math/pow.hpp>
#include <obake/math/truncate_degree.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

TEST_CASE("polynomial_diff")
{
    detail::tuple_for_each(std::make_tuple(1, mppp::integer<1>{}), [](auto n) {
        using cf_t = decltype(n);

        using pm_t = packed_monomial<long long>;
        using poly_t = polynomial<pm_t, cf_t>;

        REQUIRE(is_differentiable_v<poly_t>);
        REQUIRE(is_differentiable_v<poly_t &>);
        REQUIRE(is_differentiable_v<const poly_t &>);
        REQUIRE(is_differentiable_v<const poly_t>);

        auto [x, y, z] = make_polynomials<poly_t>("x", "y", "z");

        REQUIRE(diff(poly_t{}, "x").empty());
        REQUIRE(diff(x, "x") == 1);
        REQUIRE(diff(x, "y") == 0);
        REQUIRE(diff(x + y, "x") == 1);
        REQUIRE(diff(x + y, "y") == 1);
        REQUIRE(diff(x + y + z, "x") == 1);
        REQUIRE(diff(x + y + z, "y") == 1);
        REQUIRE(diff(x + y + z, "z") == 1);
        REQUIRE(diff(x + y + z, "zz") == 0);
        if constexpr (std::is_same_v<mppp::integer<1>, cf_t>) {
            // Try a couple of negative powers as well.
            REQUIRE(diff(x + y + obake::pow(z, -5), "z") == -5 * obake::pow(z, -6));
            REQUIRE(diff(x * z + y * z + 3 * x * y * obake::pow(z, -5), "z")
                    == x + y - 5 * 3 * x * y * obake::pow(z, -6));
        }

        auto p = 3 * x * x * y - 2 * z * y + 4 * x * z * z * z;
        REQUIRE(std::is_same_v<decltype(p), poly_t>);
        REQUIRE(diff(p, "x") == 6 * x * y + 4 * z * z * z);
        REQUIRE(diff(p, "y") == 3 * x * x - 2 * z);
        REQUIRE(diff(p, "z") == -2 * y + 4 * x * 3 * z * z);
        REQUIRE(diff(p, "zz") == 0);
        REQUIRE(diff(p, "aa") == 0);

        // Verify the return types (for the int coefficient case,
        // we will have type promotion due to the long long exponents).
        if constexpr (std::is_same_v<int, cf_t>) {
            REQUIRE(std::is_same_v<decltype(diff(p, "x")), polynomial<pm_t, long long>>);
        } else {
            REQUIRE(std::is_same_v<decltype(diff(p, "x")), poly_t>);
        }
    });

    // Recursive poly test.
    using pm_t = packed_monomial<long long>;
    using p1_t = polynomial<pm_t, mppp::integer<1>>;
    using p11_t = polynomial<pm_t, p1_t>;

    REQUIRE(is_differentiable_v<p11_t>);
    REQUIRE(is_differentiable_v<p11_t &>);
    REQUIRE(is_differentiable_v<const p11_t &>);
    REQUIRE(is_differentiable_v<const p11_t>);

    auto [x, y] = make_polynomials<p1_t>("x", "y");
    auto [z] = make_polynomials<p11_t>("z");

    auto p = 3 * x * x * y - 2 * z * y + 4 * x * z * z * z;
    REQUIRE(std::is_same_v<decltype(p), p11_t>);
    REQUIRE(diff(p, "x") == 6 * x * y + 4 * z * z * z);
    REQUIRE(diff(p, "y") == 3 * x * x - 2 * z);
    REQUIRE(diff(p, "z") == -2 * y + 4 * x * 3 * z * z);
    REQUIRE(diff(p, "zz") == 0);
    REQUIRE(diff(p, "aa") == 0);
}

TEST_CASE("polynomial_integrate")
{
    detail::tuple_for_each(std::make_tuple(1, mppp::rational<1>{}), [](auto n) {
        using cf_t = decltype(n);

        using pm_t = packed_monomial<long long>;
        using poly_t = polynomial<pm_t, cf_t>;

        {
            REQUIRE(is_integrable_v<poly_t>);
            REQUIRE(is_integrable_v<poly_t &>);
            REQUIRE(is_integrable_v<const poly_t &>);
            REQUIRE(is_integrable_v<const poly_t>);

            auto [x, y, z, zz] = make_polynomials<poly_t>("x", "y", "z", "zz");

            REQUIRE(integrate(poly_t{}, "x").empty());
            REQUIRE(integrate(poly_t{1}, "x") == x);
            REQUIRE(integrate(poly_t{2}, "x") == 2 * x);
            REQUIRE(integrate(poly_t{1}, "y") == y);
            REQUIRE(integrate(poly_t{-2}, "y") == -2 * y);

            REQUIRE(integrate(2 * x, "x") == x * x);
            REQUIRE(integrate(2 * x * y + z, "x") == x * x * y + z * x);
            REQUIRE(integrate(2 * y, "y") == y * y);
            REQUIRE(integrate(2 * x * y + z, "y") == y * y * x + z * y);
            REQUIRE(integrate(2 * z, "z") == z * z);

            // Check also some fractional results.
            if constexpr (std::is_same_v<mppp::rational<1>, cf_t>) {
                REQUIRE(integrate(x, "x") == x * x / 2);
                REQUIRE(integrate(x * y + z, "x") == x / 2 * x * y + z * x);

                // Try a couple of negative powers as well.
                REQUIRE(integrate(x + y + obake::pow(z, -5), "z") == x * z + y * z - obake::pow(z, -4) / 4);
                REQUIRE(integrate(x * z + y * z + 3 * x * y * obake::pow(z, -5), "z")
                        == x * z * z / 2 + y * z * z / 2 - 3 * x * y * obake::pow(z, -4) / 4);
            }

            auto p = 2 * x * y * z + x + y;
            REQUIRE(integrate(p, "z") == z * z * x * y + z * x + z * y);
            REQUIRE(integrate(p, "zz") == p * zz);

            // Verify the return types (for the int coefficient case,
            // we will have type promotion due to the long long exponents).
            if constexpr (std::is_same_v<int, cf_t>) {
                REQUIRE(std::is_same_v<decltype(integrate(p, "x")), polynomial<pm_t, long long>>);
            } else {
                REQUIRE(std::is_same_v<decltype(integrate(p, "x")), poly_t>);
            }
        }

        {
            // Try with polynomial coefficients whose coefficients
            // have zero derivative.
            using pp_t = polynomial<pm_t, poly_t>;
            auto [x, y, z, zz] = make_polynomials<pp_t>("x", "y", "z", "zz");
            REQUIRE(integrate(2 * x, "x") == x * x);
            REQUIRE(integrate(2 * x * y + z, "x") == x * x * y + z * x);
            REQUIRE(integrate(2 * y, "y") == y * y);
            REQUIRE(integrate(2 * x * y + z, "y") == y * y * x + z * y);
            REQUIRE(integrate(2 * z, "z") == z * z);

            auto p = 2 * x * y * z + x + y;
            REQUIRE(integrate(p, "z") == z * z * x * y + z * x + z * y);
            REQUIRE(integrate(p, "zz") == p * zz);

            // Check the error condition as well.
            auto [zp, a] = make_polynomials<poly_t>("z", "a");

            OBAKE_REQUIRES_THROWS_CONTAINS(
                integrate(p * zp, "z"), std::invalid_argument,
                "The current polynomial integration algorithm requires the derivatives of all "
                "coefficients with respect to the symbol 'z' to be zero, but a coefficient with nonzero derivative was "
                "detected");

            REQUIRE(integrate(p * a, "z") == a * (z * z * x * y + z * x + z * y));
        }
    });
}

TEST_CASE("polynomial_truncate_degree")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;
    using ppoly_t = polynomial<pm_t, poly_t>;

    REQUIRE(!is_degree_truncatable_v<poly_t, void>);
    REQUIRE(!is_degree_truncatable_v<poly_t, int>);
    REQUIRE(!is_degree_truncatable_v<poly_t, int &>);
    REQUIRE(is_degree_truncatable_v<poly_t &, int &>);
    REQUIRE(!is_degree_truncatable_v<const poly_t &, const int>);
    REQUIRE(!is_degree_truncatable_v<const poly_t, const int &>);
    // Mixed cf/key truncation not supported at this time.
    REQUIRE(!is_degree_truncatable_v<ppoly_t &, int>);

    auto [x, y, z] = make_polynomials<poly_t>("x", "y", "z");

    const auto p = x * y * z - 3 * x + 4 * x * y - z + 5;
    auto pc(p);
    truncate_degree(pc, 100);
    REQUIRE(pc == p);

    pc = p;
    truncate_degree(pc, 3);
    REQUIRE(pc == p);

    pc = p;
    truncate_degree(pc, 2);
    REQUIRE(pc == -3 * x + 4 * x * y - z + 5);

    pc = p;
    truncate_degree(pc, 1);
    REQUIRE(pc == -3 * x - z + 5);

    pc = p;
    truncate_degree(pc, 0);
    REQUIRE(pc == 5);

    pc = p;
    truncate_degree(pc, -1);
    REQUIRE(pc.empty());

    pc = p;
    truncate_degree(pc, -100);
    REQUIRE(pc.empty());
}

// Exercise the segmented tables layout.
TEST_CASE("polynomial_truncate_degree_large")
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

    auto cmp = f * g;
    const auto tcmp = truncated_mul(f, g, 50);

    truncate_degree(cmp, 50);
    REQUIRE(cmp == tcmp);
}

// A test for exercising rectangular multi-threaded multiplication.
TEST_CASE("polynomial_hm_mt_rectangular_large")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z, t, u] = make_polynomials<poly_t>(symbol_set{"x", "y", "z", "t", "u"}, "x", "y", "z", "t", "u");

    auto f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
    const auto tmp_f(f);

    poly_t tmp;
    tmp.set_symbol_set(symbol_set{"x", "y", "z", "t", "u"});

    for (int i = 1; i < 20; ++i) {
        polynomials::detail::poly_mul_impl_mt_hm(tmp, tmp_f, f);
        f = tmp;
        tmp = poly_t{};
        tmp.set_symbol_set(symbol_set{"x", "y", "z", "t", "u"});
    }

    REQUIRE(f.size() == 53130u);
}
