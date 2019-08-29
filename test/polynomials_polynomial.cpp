// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/polynomials/polynomial.hpp>
#include <piranha/symbols.hpp>

#include "catch.hpp"

using namespace piranha;

TEST_CASE("make_polynomials_test")
{
    using poly_t = polynomial<packed_monomial<long>, double>;

    REQUIRE(make_polynomials<poly_t>().size() == 0u);
    REQUIRE(make_polynomials<poly_t>(symbol_set{}).size() == 0u);

    {
        auto [a] = make_polynomials<poly_t>("a");
        REQUIRE(a.get_symbol_set() == symbol_set{"a"});
    }

    {
        auto [a1] = make_polynomials<poly_t>(symbol_set{"a"}, "a");
        REQUIRE(a1.get_symbol_set() == symbol_set{"a"});

        auto [a2] = make_polynomials<poly_t>(symbol_set{"a", "b", "c"}, "a");
        REQUIRE(a2.get_symbol_set() == symbol_set{"a", "b", "c"});

        auto [b, c] = make_polynomials<poly_t>(symbol_set{"a", "b", "c"}, "b", "c");
        REQUIRE(b.get_symbol_set() == symbol_set{"a", "b", "c"});
        REQUIRE(c.get_symbol_set() == symbol_set{"a", "b", "c"});
    }
}
