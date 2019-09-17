// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

// #include <type_traits>

#include <mp++/rational.hpp>

// #include <piranha/byte_size.hpp>
// #include <piranha/config.hpp>
// #include <piranha/math/degree.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/polynomials/packed_monomial.hpp>
// #include <piranha/polynomials/polynomial.hpp>
#include <piranha/series.hpp>
// #include <piranha/symbols.hpp>

#include "catch.hpp"

using rat_t = mppp::rational<1>;

using namespace piranha;

TEST_CASE("series_pow_test")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    std::cout << piranha::pow(s1_t{rat_t{4, 3}}, 3.2) << '\n';
}
