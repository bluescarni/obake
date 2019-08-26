// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <bitset>
#include <cstdint>
#include <iostream>

#include <piranha/config.hpp>
#include <piranha/detail/xoroshiro128_plus.hpp>

#include "catch.hpp"

using namespace piranha;

static detail::xoroshiro128_plus rng{12724899751400538854ull, 9282269007213506749ull};

TEST_CASE("random_test")
{
    std::cout << "Random 32bit number : " << std::bitset<32>(rng.random<std::uint32_t>()) << '\n';
    std::cout << "Random 64bit number : " << std::bitset<64>(rng.random<std::uint64_t>()) << '\n';

#if defined(PIRANHA_HAVE_GCC_INT128)
    const auto r = rng.random<__uint128_t>();
    std::cout << "Random 128bit number: " << std::bitset<64>(static_cast<std::uint64_t>(r >> 64))
              << std::bitset<64>(static_cast<std::uint64_t>((r << 64) >> 64)) << '\n';
#endif
}
