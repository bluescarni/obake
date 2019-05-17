// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/symbols.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace piranha;

TEST_CASE("symbol_set_to_string_test")
{
    REQUIRE(detail::to_string(symbol_set{}) == "{}");
    REQUIRE(detail::to_string(symbol_set{"b"}) == "{'b'}");
    REQUIRE(detail::to_string(symbol_set{"b", "a"}) == "{'a', 'b'}");
    REQUIRE(detail::to_string(symbol_set{"c", "b", "a"}) == "{'a', 'b', 'c'}");
    REQUIRE(detail::to_string(symbol_set{"a", "a", "a"}) == "{'a'}");
}
