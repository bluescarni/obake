// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/math/pow.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

struct foo {
};

TEST_CASE("customisation")
{
    std::cout << n<int, int> << '\n';
    std::cout << n<float, int> << '\n';
    // foo f1, f2;
    // REQUIRE(piranha::pow(f1, f2) == 0);
}
