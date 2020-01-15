// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
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

#include <obake/detail/ignore.hpp>
#include <obake/math/evaluate.hpp>
#include <obake/math/pow.hpp>
#include <obake/math/trim.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_name.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using rat_t = mppp::rational<1>;

using namespace obake;

TEST_CASE("series_pow_test")
{
    obake_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using p1_t = polynomial<pm_t, rat_t>;

    REQUIRE(is_exponentiable_v<s1_t, int>);
    REQUIRE(is_exponentiable_v<s1_t &, const int>);
    REQUIRE(is_exponentiable_v<s1_t &&, const int &>);
    REQUIRE(!is_exponentiable_v<s1_t, void>);
    REQUIRE(!is_exponentiable_v<int, s1_t>);

    REQUIRE(std::is_same_v<series<pm_t, rat_t, void>, decltype(obake::pow(s1_t{}, 0))>);
    REQUIRE(std::is_same_v<series<pm_t, double, void>, decltype(obake::pow(s1_t{}, 0.))>);
    REQUIRE(std::is_same_v<series<pm_t, float, void>, decltype(obake::pow(s1_t{}, 0.f))>);

    // Test single_cf() implementation.
    REQUIRE(obake::pow(s1_t{}, 10) == 0);
    REQUIRE(obake::pow(s1_t{}, 0) == 1);
    REQUIRE(obake::pow(s1_t{rat_t{3, 5}}, 2) == rat_t{9, 25});
    REQUIRE(obake::pow(s1_t{rat_t{3, 5}}, -2) == rat_t{25, 9});

    REQUIRE(obake::pow(s1_t{}, 10.) == 0.);
    REQUIRE(obake::pow(s1_t{}, 0.) == 1.);
    REQUIRE(obake::pow(s1_t{rat_t{3, 2}}, 2) == std::pow(3. / 2., 2));
    REQUIRE(obake::pow(s1_t{rat_t{2, 5}}, -2) == std::pow(5. / 2., 2));

    // Anything to the zero is 1.
    auto [x, y] = make_polynomials<p1_t>("x", "y");

    REQUIRE(obake::pow(x - y, 0) == 1);
    REQUIRE(obake::pow(x, 0) == 1);
    REQUIRE(obake::pow((x - y) * (x + y), 0) == 1);

    // Exponentiation via repeated multiplications.
    customisation::internal::series_default_pow_impl impl;

    REQUIRE(impl(x - y, 1) == x - y);
    REQUIRE(impl(x - y, 2) == (x - y) * (x - y));
    REQUIRE(impl(x - y, rat_t{3}) == (x - y) * (x - y) * (x - y));
    REQUIRE(impl(x - y, rat_t{10}) == impl(x - y, rat_t{5}) * impl(x - y, rat_t{5}));
    REQUIRE(impl(x - y, mppp::integer<1>{10}) == impl(x - y, mppp::integer<1>{5}) * impl(x - y, mppp::integer<1>{5}));

    // Error handling.
    OBAKE_REQUIRES_THROWS_CONTAINS(
        impl(x - y, rat_t{1, 2}), std::invalid_argument,
        "Invalid exponent for series exponentiation via repeated "
        "multiplications: the exponent (1/2) cannot be converted into a non-negative integral value");
    OBAKE_REQUIRES_THROWS_CONTAINS(
        impl(x - y, -1), std::invalid_argument,
        "Invalid exponent for series exponentiation via repeated "
        "multiplications: the exponent (-1) cannot be converted into a non-negative integral value");

    s1_t a;
    a.set_symbol_set(symbol_set{"a"});
    a.add_term(pm_t{1}, 1, 2);

    OBAKE_REQUIRES_THROWS_CONTAINS(
        impl(a, 5), std::invalid_argument,
        "Cannot compute the power of a series of type '" + type_name<s1_t>()
            + "': the series does not consist of a single coefficient, "
              "and exponentiation via repeated multiplications is not possible (either because the "
              "exponent cannot be converted to a non-negative integral value, or because the "
              "series/coefficient types do not support the necessary operations)");

    // Test clearing of the cache.
    auto [map, _] = customisation::internal::get_series_pow_map();
    detail::ignore(_);

    REQUIRE(!map.empty());

    customisation::internal::clear_series_pow_map();

    REQUIRE(map.empty());
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
    REQUIRE(evaluate(x * y - obake::pow(z, 3) * 4, symbol_map<double>{{"x", 1.}, {"y", 2.}, {"z", 3.}}) == -106);
    REQUIRE(evaluate(x * y - obake::pow(z, 3) * 4, symbol_map<mppp::integer<1>>{{"x", mppp::integer<1>{1}},
                                                                                {"y", mppp::integer<1>{2}},
                                                                                {"z", mppp::integer<1>{3}}})
            == -106);
    REQUIRE(std::abs(evaluate(x * y - obake::pow(z, -3) * 4, symbol_map<double>{{"x", 1.}, {"y", 2.}, {"z", 3.}})
                     - 1.851851851)
            < 1e-8);
    REQUIRE(evaluate(x * y - obake::pow(z, -3) * 4, symbol_map<mppp::rational<1>>{{"x", mppp::rational<1>(1)},
                                                                                  {"y", mppp::rational<1>(2)},
                                                                                  {"z", mppp::rational<1>(3)}})
            == mppp::rational<1>{50, 27});
    REQUIRE(evaluate(x * y - obake::pow(z, -3) * 4 + 6 * y * z * x - 3 * obake::pow(x, -1),
                     symbol_map<mppp::rational<1>>{
                         {"x", mppp::rational<1>(1)}, {"y", mppp::rational<1>(2)}, {"z", mppp::rational<1>(3)}})
            == mppp::rational<1>{941, 27});
    REQUIRE(evaluate(x * y - obake::pow(z, -3) * 4 + 6 * y * z * x - 3 * obake::pow(x, -1),
                     symbol_map<p1_t>{{"x", p1_t(1)}, {"y", p1_t(2)}, {"z", p1_t(3)}})
            == p1_t{mppp::rational<1>{941, 27}});
    REQUIRE(evaluate(x * y - obake::pow(z, 3) * 4, symbol_map<p2_t>{{"x", p2_t{1}}, {"y", p2_t{2}}, {"z", p2_t{3}}})
            == -106);
    REQUIRE(evaluate(obake::pow(z, -3), symbol_map<p2_t>{{"z", p2_t{3}}}) == 0);

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

    OBAKE_REQUIRES_THROWS_CONTAINS(
        evaluate(x * y - obake::pow(z, 3) * 4, symbol_map<double>{{"x", 1.}, {"y", 2.}}), std::invalid_argument,
        "Cannot evaluate a series: the evaluation map, which contains the symbols {'x', 'y'}, does not contain "
        "all the symbols in the series' symbol set, {'x', 'y', 'z'}");
    OBAKE_REQUIRES_THROWS_CONTAINS(
        evaluate(x * y - obake::pow(z, 3) * 4, symbol_map<double>{{"x", 1.}, {"y", 2.}, {"t", 4.5}}),
        std::invalid_argument,
        "Cannot evaluate a series: the evaluation map, which contains the symbols {'t', 'x', 'y'}, does not contain "
        "all the symbols in the series' symbol set, {'x', 'y', 'z'}");
    OBAKE_REQUIRES_THROWS_CONTAINS(
        evaluate(x * y - obake::pow(z, 3) * 4, symbol_map<double>{{"x", 1.}, {"y", 2.}, {"t", 4.5}, {"u", 0.}}),
        std::invalid_argument,
        "Cannot evaluate a series: the evaluation map, which contains the symbols {'t', 'u', 'x', 'y'}, does not "
        "contain all the symbols in the series' symbol set, {'x', 'y', 'z'}");
}

TEST_CASE("series_trim_test")
{
    using pm_t = packed_monomial<int>;
    using p1_t = polynomial<pm_t, rat_t>;

    REQUIRE(is_trimmable_v<p1_t>);
    REQUIRE(is_trimmable_v<p1_t &>);
    REQUIRE(is_trimmable_v<const p1_t &>);
    REQUIRE(is_trimmable_v<const p1_t>);

    auto [x, y, z] = make_polynomials<p1_t>("x", "y", "z");

    REQUIRE(trim(x) == x);
    REQUIRE(trim(x).get_symbol_set() == x.get_symbol_set());

    auto p1 = x * x + 2 * y - 3 * z;
    REQUIRE(trim(p1) == p1);
    REQUIRE(trim(p1).get_symbol_set() == p1.get_symbol_set());

    auto p2 = p1 * p1 * p1 * p1;
    REQUIRE(trim(p2) == p2);
    REQUIRE(trim(p2).get_symbol_set() == p2.get_symbol_set());

    auto p3 = x * x + 2 * y - 3 * z + 3 * z;
    REQUIRE(trim(p3) == p3);
    REQUIRE(trim(p3).get_symbol_set() != p3.get_symbol_set());
    REQUIRE(trim(p3).get_symbol_set() == symbol_set{"x", "y"});

    auto p4 = x * x + 2 * y - 3 * z + 3 * z - 2 * y;
    REQUIRE(trim(p4) == p4);
    REQUIRE(trim(p4).get_symbol_set() != p4.get_symbol_set());
    REQUIRE(trim(p4).get_symbol_set() == symbol_set{"x"});

    auto p5 = x * x + 2 * y - 3 * z + 3 * z - 2 * y - x * x;
    REQUIRE(p5.empty());
    REQUIRE(trim(p5) == p5);
    REQUIRE(trim(p5).get_symbol_set() != p5.get_symbol_set());
    REQUIRE(trim(p5).get_symbol_set() == symbol_set{});
}
