// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cmath>
#include <stdexcept>
#include <type_traits>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

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
