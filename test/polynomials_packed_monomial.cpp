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

#include <initializer_list>

#include <piranha/key/key_is_zero.hpp>
#include <piranha/symbols.hpp>

using namespace piranha;

TEST_CASE("ctor_test")
{
    int arr[] = {1, 2};
    using pm_t = packed_monomial<int>;
    [[maybe_unused]] pm_t pm0(arr, 2), pm1(arr), pm2{1, 2};

    REQUIRE(!key_is_zero(pm_t{}, symbol_set{}));
    REQUIRE(is_zero_testable_key_v<pm_t>);
}
