// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/series.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <type_traits>

#include <piranha/math/pow.hpp>

TEST_CASE("pow_test")
{
    using series_t = piranha::series<void, void, void>;

    REQUIRE(std::is_same_v<decltype(piranha::pow(series_t{}, 0)), int>);
    REQUIRE(piranha::pow(series_t{}, 0) == 0);
    REQUIRE((!piranha::is_exponentiable_v<series_t, double>));
}
