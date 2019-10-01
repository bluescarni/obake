// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <sstream>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/tex_stream_insert.hpp>

#include "catch.hpp"

using int_t = mppp::integer<1>;
using rat_t = mppp::rational<1>;

using namespace obake;

TEST_CASE("series_tex_stream_test")
{
    using pm_t = packed_monomial<int>;
    using p1_t = polynomial<pm_t, rat_t>;
    // using p2_t = polynomial<pm_t, int_t>;

    auto [x, y, z] = make_polynomials<p1_t>("x", "y", "z");

    std::ostringstream oss;

    tex_stream_insert(oss, x / 2);
    REQUIRE(oss.str() == "\\frac{1}{2}{x}");
}
