// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/containers/hash_map.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <array>

using namespace piranha;

TEST_CASE("basic")
{
    hash_map<int, int> hm0;
    std::cout << hm0.max_size() << '\n';
    REQUIRE(hm0.empty());
    hash_map<int, std::array<int, (1ul << 20)>> hm1;
    std::cout << hm1.max_size() << '\n';
    REQUIRE(hm1.empty());
}
