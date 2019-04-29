// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/hash.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <functional>
#include <string>

using namespace piranha;

TEST_CASE("hash_test")
{
    REQUIRE(!is_hashable_v<void>);

    REQUIRE(is_hashable_v<int>);
    REQUIRE(is_hashable_v<int &>);
    REQUIRE(is_hashable_v<const int &>);
    REQUIRE(is_hashable_v<int &&>);
    REQUIRE(hash(42) == std::hash<int>{}(42));

    REQUIRE(is_hashable_v<std::string>);
    REQUIRE(is_hashable_v<std::string &>);
    REQUIRE(is_hashable_v<const std::string &>);
    REQUIRE(is_hashable_v<std::string &&>);
    REQUIRE(hash(std::string{"hello world"}) == std::hash<std::string>{}(std::string{"hello world"}));
}
