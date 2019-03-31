// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/detail/bit_packing.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace piranha::detail;

TEST_CASE("bit_packer")
{
    bit_packer<unsigned> bp0(5);
    // bp0 << 1 << 2 << 3 << 4 << 5 << 6;
}
