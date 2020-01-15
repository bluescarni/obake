// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>

#include <obake/detail/hc.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("hc_test")
{
    REQUIRE(detail::hc() > 0u);
    std::cout << "The detected hardware concurrency is: " << detail::hc() << '\n';
}
