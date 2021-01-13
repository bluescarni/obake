// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdint>

#include <obake/polynomials/packed_monomial.hpp>
#include <obake/power_series/power_series.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("basic")
{
    p_series<packed_monomial<std::int32_t>, double> foo;

    set_truncation(foo, 5, {"x", "y", "z"});
    std::cout << foo << '\n';

    unset_truncation(foo);
    std::cout << foo << '\n';
}
