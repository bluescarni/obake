// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/polynomials/polynomial.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <piranha/math/pow.hpp>
#include <piranha/polynomials/packed_monomial.hpp>

using namespace piranha;

TEST_CASE("mul_test")
{
    using pm_t = packed_monomial<int>;

    using poly_t = polynomial<pm_t, double>;

    std::cout << poly_t{} * poly_t{} << '\n';
    std::cout << poly_t{3} * 3 << '\n';
    std::cout << 3 * poly_t{3} << '\n';
}
