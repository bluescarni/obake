// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <initializer_list>
#include <type_traits>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <obake/math/subs.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/symbols.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("polynomial_subs_test")
{
    using int1_t = mppp::integer<1>;
    using rat1_t = mppp::rational<1>;
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, int1_t>;
    using poly2_t = polynomial<pm_t, double>;
    using poly3_t = polynomial<pm_t, rat1_t>;

    REQUIRE(!is_substitutable_v<poly_t, void>);
    REQUIRE(!is_substitutable_v<poly_t, int &>);
    REQUIRE(!is_substitutable_v<poly_t, const int &>);
    REQUIRE(!is_substitutable_v<poly_t, const int>);
    REQUIRE(is_substitutable_v<poly_t, int>);
    REQUIRE(is_substitutable_v<poly_t, int1_t>);
    REQUIRE(is_substitutable_v<poly_t &, int>);
    REQUIRE(is_substitutable_v<const poly_t, int>);
    REQUIRE(is_substitutable_v<const poly_t &, int>);

    REQUIRE(subs(poly_t{}, symbol_map<int1_t>{}).empty());
    REQUIRE(subs(poly_t{}, symbol_map<int1_t>{{"x", int1_t{1}}}).empty());
    REQUIRE(subs(poly_t{}, symbol_map<int1_t>{{"x", int1_t{1}}, {"y", int1_t{2}}}).empty());

    auto [x, y, z] = make_polynomials<poly_t>("x", "y", "z");
    const auto p = x * y * z - 3 * x + 4 * y + 5 * x * y + y * y;

    REQUIRE(std::is_same_v<poly_t, decltype(subs(p, symbol_map<int1_t>{}))>);
    REQUIRE(std::is_same_v<poly2_t, decltype(subs(p, symbol_map<double>{}))>);
    REQUIRE(std::is_same_v<poly3_t, decltype(subs(p, symbol_map<rat1_t>{}))>);
    REQUIRE(std::is_same_v<poly_t, decltype(subs(p, symbol_map<poly_t>{}))>);

    // Substitution with an int returns the original value
    // because we don't define pow() over C++ integrals.
    REQUIRE(subs(p, symbol_map<int>{{"x", 3}}) == p);

    REQUIRE(subs(p, symbol_map<int1_t>{{"x", int1_t{3}}}) == 3 * y * z - 3 * 3 + 4 * y + 5 * 3 * y + y * y);
    REQUIRE(subs(p, symbol_map<int1_t>{{"y", int1_t{-4}}}) == x * -4 * z - 3 * x + 4 * -4 + 5 * x * -4 + 16);
    REQUIRE(subs(p, symbol_map<int1_t>{{"z", int1_t{0}}}) == -3 * x + 4 * y + 5 * x * y + y * y);
    REQUIRE(subs(p, symbol_map<int1_t>{{"x", int1_t{3}}, {"z", int1_t{0}}}) == -3 * 3 + 4 * y + 5 * 3 * y + y * y);
    REQUIRE(subs(p, symbol_map<int1_t>{{"y", int1_t{-4}}, {"z", int1_t{0}}}) == -3 * x + 4 * -4 + 5 * x * -4 + 16);
    REQUIRE(subs(p, symbol_map<int1_t>{{"x", int1_t{3}}, {"z", int1_t{0}}}) == -3 * 3 + 4 * y + 5 * 3 * y + y * y);
    REQUIRE(subs(p, symbol_map<int1_t>{{"x", int1_t{3}}, {"y", int1_t{-4}}, {"z", int1_t{0}}})
            == -3 * 3 + 4 * -4 + 5 * 3 * -4 + 16);

    REQUIRE(subs(p, symbol_map<double>{{"x", double{3}}}) == 3 * y * z - 3 * 3 + 4 * y + 5 * 3 * y + y * y);
    REQUIRE(subs(p, symbol_map<double>{{"y", double{-4}}}) == x * -4 * z - 3 * x + 4 * -4 + 5 * x * -4 + 16);
    REQUIRE(subs(p, symbol_map<double>{{"z", double{0}}}) == -3 * x + 4 * y + 5 * x * y + y * y);
    REQUIRE(subs(p, symbol_map<double>{{"x", double{3}}, {"z", double{0}}}) == -3 * 3 + 4 * y + 5 * 3 * y + y * y);
    REQUIRE(subs(p, symbol_map<double>{{"y", double{-4}}, {"z", double{0}}}) == -3 * x + 4 * -4 + 5 * x * -4 + 16);
    REQUIRE(subs(p, symbol_map<double>{{"x", double{3}}, {"z", double{0}}}) == -3 * 3 + 4 * y + 5 * 3 * y + y * y);
    REQUIRE(subs(p, symbol_map<double>{{"x", double{3}}, {"y", double{-4}}, {"z", double{0}}})
            == -3 * 3 + 4 * -4 + 5 * 3 * -4 + 16);

    REQUIRE(subs(p, symbol_map<rat1_t>{{"x", rat1_t{3}}}) == 3 * y * z - 3 * 3 + 4 * y + 5 * 3 * y + y * y);
    REQUIRE(subs(p, symbol_map<rat1_t>{{"y", rat1_t{-4}}}) == x * -4 * z - 3 * x + 4 * -4 + 5 * x * -4 + 16);
    REQUIRE(subs(p, symbol_map<rat1_t>{{"z", rat1_t{0}}}) == -3 * x + 4 * y + 5 * x * y + y * y);
    REQUIRE(subs(p, symbol_map<rat1_t>{{"x", rat1_t{3}}, {"z", rat1_t{0}}}) == -3 * 3 + 4 * y + 5 * 3 * y + y * y);
    REQUIRE(subs(p, symbol_map<rat1_t>{{"y", rat1_t{-4}}, {"z", rat1_t{0}}}) == -3 * x + 4 * -4 + 5 * x * -4 + 16);
    REQUIRE(subs(p, symbol_map<rat1_t>{{"x", rat1_t{3}}, {"z", rat1_t{0}}}) == -3 * 3 + 4 * y + 5 * 3 * y + y * y);
    REQUIRE(subs(p, symbol_map<rat1_t>{{"x", rat1_t{3}}, {"y", rat1_t{-4}}, {"z", rat1_t{0}}})
            == -3 * 3 + 4 * -4 + 5 * 3 * -4 + 16);

    // Test subs with a polynomial.
    REQUIRE(subs(p, symbol_map<poly_t>{{"x", 3 * x}}) == 3 * x * y * z - 3 * 3 * x + 4 * y + 5 * 3 * x * y + y * y);
    REQUIRE(subs(p, symbol_map<poly_t>{{"x", 3 * x}, {"y", -y}})
            == 3 * x * -y * z - 3 * 3 * x + 4 * -y + 5 * 3 * x * -y + -y * -y);
    REQUIRE(subs(p, symbol_map<poly_t>{{"x", 3 * x}, {"y", -y}, {"z", x * y}})
            == 3 * x * -y * x * y - 3 * 3 * x + 4 * -y + 5 * 3 * x * -y + -y * -y);
}
