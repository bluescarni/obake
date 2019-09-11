// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <mp++/rational.hpp>

#include <piranha/byte_size.hpp>
#include <piranha/config.hpp>
#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/series.hpp>
#include <piranha/symbols.hpp>

#include "catch.hpp"

using rat_t = mppp::rational<1>;

using namespace piranha;

TEST_CASE("series_byte_size")
{
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
