// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/series.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <mp++/rational.hpp>

#include <piranha/polynomials/packed_monomial.hpp>

#include "test_utils.hpp"

using rat_t = mppp::rational<1>;

using namespace piranha;

TEST_CASE("series_lookup")
{
    piranha_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    REQUIRE(s1.find(pm_t{}) == s1.end());
    REQUIRE(static_cast<const s1_t &>(s1).find(pm_t{}) == s1.cend());
}
