// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/polynomials/packed_monomial.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace piranha;

TEST_CASE("ctor_test")
{
    int arr[] = {1, 2};
    [[maybe_unused]] packed_monomial<int> pm0(arr, 2), pm1(arr), pm2{1, 2};
}
