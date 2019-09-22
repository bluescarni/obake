// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <piranha/math/evaluate.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/polynomials/polynomial.hpp>
#include <piranha/series.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_name.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using rat_t = mppp::rational<1>;

using namespace piranha;

TEST_CASE("series_pow_test")
{
    piranha_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using p1_t = polynomial<pm_t, rat_t>;

    REQUIRE(is_exponentiable_v<s1_t, int>);
    REQUIRE(is_exponentiable_v<s1_t &, const int>);
    REQUIRE(is_exponentiable_v<s1_t &&, const int &>);
    REQUIRE(!is_exponentiable_v<s1_t, void>);
    REQUIRE(!is_exponentiable_v<int, s1_t>);

    REQUIRE(std::is_same_v<series<pm_t, rat_t, void>, decltype(piranha::pow(s1_t{}, 0))>);
    REQUIRE(std::is_same_v<series<pm_t, double, void>, decltype(piranha::pow(s1_t{}, 0.))>);
    REQUIRE(std::is_same_v<series<pm_t, float, void>, decltype(piranha::pow(s1_t{}, 0.f))>);

    // Test single_cf() implementation.
    REQUIRE(piranha::pow(s1_t{}, 10) == 0);
    REQUIRE(piranha::pow(s1_t{}, 0) == 1);
    REQUIRE(piranha::pow(s1_t{rat_t{3, 5}}, 2) == rat_t{9, 25});
    REQUIRE(piranha::pow(s1_t{rat_t{3, 5}}, -2) == rat_t{25, 9});

    REQUIRE(piranha::pow(s1_t{}, 10.) == 0.);
    REQUIRE(piranha::pow(s1_t{}, 0.) == 1.);
    REQUIRE(piranha::pow(s1_t{rat_t{3, 2}}, 2) == std::pow(3. / 2., 2));
    REQUIRE(piranha::pow(s1_t{rat_t{2, 5}}, -2) == std::pow(5. / 2., 2));

    // Anything to the zero is 1.
    auto [x, y] = make_polynomials<p1_t>("x", "y");

    REQUIRE(piranha::pow(x - y, 0) == 1);
    REQUIRE(piranha::pow(x, 0) == 1);
    REQUIRE(piranha::pow((x - y) * (x + y), 0) == 1);

    // Exponentiation via repeated multiplications.
    customisation::internal::series_default_pow_impl impl;

    REQUIRE(impl(x - y, 1) == x - y);
    REQUIRE(impl(x - y, 2) == (x - y) * (x - y));
    REQUIRE(impl(x - y, rat_t{3}) == (x - y) * (x - y) * (x - y));
    REQUIRE(impl(x - y, rat_t{10}) == impl(x - y, rat_t{5}) * impl(x - y, rat_t{5}));
    REQUIRE(impl(x - y, mppp::integer<1>{10}) == impl(x - y, mppp::integer<1>{5}) * impl(x - y, mppp::integer<1>{5}));

    // Error handling.
    PIRANHA_REQUIRES_THROWS_CONTAINS(impl(x - y, rat_t{1, 2}), std::invalid_argument,
                                     "Invalid exponent for series exponentiation via repeated "
                                     "multiplications: the exponent (1/2) cannot be converted into an integral value");
    PIRANHA_REQUIRES_THROWS_CONTAINS(impl(x - y, -1), std::invalid_argument,
                                     "Invalid exponent for series exponentiation via repeated "
                                     "multiplications: the exponent (-1) is negative");

    s1_t a;
    a.set_symbol_set(symbol_set{"a"});
    a.add_term(pm_t{1}, 1, 2);

    PIRANHA_REQUIRES_THROWS_CONTAINS(
        impl(a, 5), std::invalid_argument,
        "Cannot compute the power of a series of type '" + type_name<s1_t>()
            + "': the series does not consist of a single coefficient, "
              "and exponentiation via repeated multiplications is not possible (either because the "
              "exponent cannot be converted to an integral value, or because the series type does "
              "not support the necessary arithmetic operations)");
}

TEST_CASE("series_evaluate_test")
{
    using pm_t = packed_monomial<int>;
    using p1_t = polynomial<pm_t, rat_t>;
    using p2_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z] = make_polynomials<p1_t>("x", "y", "z");

    REQUIRE(evaluate(p1_t{}, symbol_map<double>{}) == 0);
    REQUIRE(evaluate(p1_t{}, symbol_map<mppp::integer<1>>{}) == 0);
    REQUIRE(evaluate(p1_t{}, symbol_map<mppp::rational<1>>{}) == 0);
    REQUIRE(evaluate(p2_t{}, symbol_map<double>{}) == 0);
    REQUIRE(evaluate(p2_t{}, symbol_map<mppp::integer<1>>{}) == 0);
    REQUIRE(evaluate(p2_t{}, symbol_map<mppp::rational<1>>{}) == 0);

    REQUIRE(std::is_same_v<double, decltype(evaluate(p1_t{}, symbol_map<double>{}))>);
    REQUIRE(std::is_same_v<double, decltype(evaluate(p2_t{}, symbol_map<double>{}))>);
    REQUIRE(std::is_same_v<float, decltype(evaluate(p1_t{}, symbol_map<float>{}))>);
    REQUIRE(std::is_same_v<float, decltype(evaluate(p2_t{}, symbol_map<float>{}))>);
    REQUIRE(std::is_same_v<mppp::rational<1>, decltype(evaluate(p1_t{}, symbol_map<mppp::integer<1>>{}))>);
    REQUIRE(std::is_same_v<mppp::integer<1>, decltype(evaluate(p2_t{}, symbol_map<mppp::integer<1>>{}))>);
    REQUIRE(std::is_same_v<p1_t, decltype(evaluate(p1_t{}, symbol_map<p1_t>{}))>);
    REQUIRE(std::is_same_v<p1_t, decltype(evaluate(p2_t{}, symbol_map<p1_t>{}))>);

    REQUIRE(evaluate(p1_t{3}, symbol_map<double>{}) == 3);
    REQUIRE(evaluate(p1_t{-42}, symbol_map<double>{}) == -42);
    REQUIRE(evaluate(x * y - piranha::pow(z, 3) * 4, symbol_map<double>{{"x", 1.}, {"y", 2.}, {"z", 3.}}) == -106);
    REQUIRE(evaluate(x * y - piranha::pow(z, 3) * 4, symbol_map<mppp::integer<1>>{{"x", mppp::integer<1>{1}},
                                                                                  {"y", mppp::integer<1>{2}},
                                                                                  {"z", mppp::integer<1>{3}}})
            == -106);
    REQUIRE(std::abs(evaluate(x * y - piranha::pow(z, -3) * 4, symbol_map<double>{{"x", 1.}, {"y", 2.}, {"z", 3.}})
                     - 1.851851851)
            < 1e-8);
    REQUIRE(evaluate(x * y - piranha::pow(z, -3) * 4, symbol_map<mppp::rational<1>>{{"x", mppp::rational<1>(1)},
                                                                                    {"y", mppp::rational<1>(2)},
                                                                                    {"z", mppp::rational<1>(3)}})
            == mppp::rational<1>{50, 27});
    REQUIRE(evaluate(x * y - piranha::pow(z, -3) * 4 + 6 * y * z * x - 3 * piranha::pow(x, -1),
                     symbol_map<mppp::rational<1>>{
                         {"x", mppp::rational<1>(1)}, {"y", mppp::rational<1>(2)}, {"z", mppp::rational<1>(3)}})
            == mppp::rational<1>{941, 27});
    REQUIRE(evaluate(x * y - piranha::pow(z, -3) * 4 + 6 * y * z * x - 3 * piranha::pow(x, -1),
                     symbol_map<p1_t>{{"x", p1_t(1)}, {"y", p1_t(2)}, {"z", p1_t(3)}})
            == p1_t{mppp::rational<1>{941, 27}});
    REQUIRE(evaluate(x * y - piranha::pow(z, 3) * 4, symbol_map<p2_t>{{"x", p2_t{1}}, {"y", p2_t{2}}, {"z", p2_t{3}}})
            == -106);
    REQUIRE(evaluate(piranha::pow(z, -3), symbol_map<p2_t>{{"z", p2_t{3}}}) == 0);

    // Test the type trait.
    REQUIRE(!is_evaluable_v<p1_t, void>);
    REQUIRE(!is_evaluable_v<p1_t, double &>);
    REQUIRE(!is_evaluable_v<p1_t, const double>);
    REQUIRE(!is_evaluable_v<p2_t, void>);
    REQUIRE(!is_evaluable_v<p2_t, double &>);
    REQUIRE(!is_evaluable_v<p2_t, const double>);
    REQUIRE(is_evaluable_v<p1_t, double>);
    REQUIRE(is_evaluable_v<p1_t &, double>);
    REQUIRE(is_evaluable_v<const p1_t &, double>);
    REQUIRE(is_evaluable_v<const p1_t, double>);
    REQUIRE(is_evaluable_v<p2_t, double>);
    REQUIRE(is_evaluable_v<p2_t &, double>);
    REQUIRE(is_evaluable_v<const p2_t &, double>);
    REQUIRE(is_evaluable_v<const p2_t, double>);

    PIRANHA_REQUIRES_THROWS_CONTAINS(
        evaluate(x * y - piranha::pow(z, 3) * 4, symbol_map<double>{{"x", 1.}, {"y", 2.}}), std::invalid_argument,
        "Cannot evaluate a series: the evaluation map, which contains the symbols {'x', 'y'}, does not contain "
        "all the symbols in the series' symbol set, {'x', 'y', 'z'}");
    PIRANHA_REQUIRES_THROWS_CONTAINS(
        evaluate(x * y - piranha::pow(z, 3) * 4, symbol_map<double>{{"x", 1.}, {"y", 2.}, {"t", 4.5}}),
        std::invalid_argument,
        "Cannot evaluate a series: the evaluation map, which contains the symbols {'t', 'x', 'y'}, does not contain "
        "all the symbols in the series' symbol set, {'x', 'y', 'z'}");
    PIRANHA_REQUIRES_THROWS_CONTAINS(
        evaluate(x * y - piranha::pow(z, 3) * 4, symbol_map<double>{{"x", 1.}, {"y", 2.}, {"t", 4.5}, {"u", 0.}}),
        std::invalid_argument,
        "Cannot evaluate a series: the evaluation map, which contains the symbols {'t', 'u', 'x', 'y'}, does not "
        "contain all the symbols in the series' symbol set, {'x', 'y', 'z'}");
}
