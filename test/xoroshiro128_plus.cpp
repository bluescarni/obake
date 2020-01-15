// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <bitset>
#include <cstdint>
#include <iostream>
#include <random>

#include <obake/config.hpp>
#include <obake/detail/xoroshiro128_plus.hpp>

#include "catch.hpp"

using namespace obake;

static detail::xoroshiro128_plus rng{12724899751400538854ull, 9282269007213506749ull};

TEST_CASE("random_test")
{
    std::cout << "Random 32bit number : " << std::bitset<32>(rng.random<std::uint32_t>()) << '\n';
    std::cout << "Random 64bit number : " << std::bitset<64>(rng.random<std::uint64_t>()) << '\n';

#if defined(OBAKE_HAVE_GCC_INT128)
    const auto r = rng.random<__uint128_t>();
    std::cout << "Random 128bit number: " << std::bitset<64>(static_cast<std::uint64_t>(r >> 64))
              << std::bitset<64>(static_cast<std::uint64_t>((r << 64) >> 64)) << '\n';
#endif
}

// Check we can use detail::xoroshiro128_plus as a random engine
// for the standard library facilities.
TEST_CASE("cpp_random_interface_test")
{
    std::cout << "Ten random integers:\n";
    std::uniform_int_distribution<int> idist(0, 100);
    for (auto i = 0; i < 10; ++i) {
        std::cout << idist(rng) << '\n';
    }

    std::cout << "Ten random floats:\n";
    std::uniform_real_distribution<double> rdist(0., 1.);
    for (auto i = 0; i < 10; ++i) {
        std::cout << rdist(rng) << '\n';
    }
}
