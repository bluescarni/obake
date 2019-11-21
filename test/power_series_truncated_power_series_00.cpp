// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>

#include <mp++/rational.hpp>

#include <obake/math/degree.hpp>
#include <obake/math/p_degree.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/power_series/truncated_power_series.hpp>
#include <obake/symbols.hpp>

#include "catch.hpp"

using namespace obake;

struct foo {
};

TEST_CASE("basic_test")
{
    using tps_t = truncated_power_series<packed_monomial<int>, mppp::rational<1>>;

    tps_t t00;
    REQUIRE(t00._poly().empty());
    REQUIRE(degree(t00) == 0);
    REQUIRE(p_degree(t00, symbol_set{}) == 0);

    tps_t{45};
    tps_t{std::string("423423")};

    REQUIRE(!std::is_constructible_v<tps_t, foo>);

    std::cout << tps_t{45} << '\n';
    std::cout << tps_t{45, 3} << '\n';
    std::cout << tps_t{45, 3u} << '\n';
    std::cout << tps_t{45, 3u, symbol_set{"x", "y", "z"}} << '\n';

    tps_t a{45};
    a = -42;
    std::cout << a << '\n';
}
