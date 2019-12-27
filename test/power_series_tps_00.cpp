// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>

#include <mp++/rational.hpp>

#include <obake/polynomials/packed_monomial.hpp>
#include <obake/power_series/tps.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("tps_basic_tests")
{
    using tps_t = tps<packed_monomial<int>, mppp::rational<1>>;

    tps_t t00;
}
