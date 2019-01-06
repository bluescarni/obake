// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/math/pow.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cmath>
#include <type_traits>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>

#include <piranha/config.hpp>

TEST_CASE("pow_arith")
{
    REQUIRE(piranha::is_exponentiable_v<float, int>);
    REQUIRE(piranha::is_exponentiable_v<double, float>);
    REQUIRE(piranha::is_exponentiable_v<const double &, long &&>);
    REQUIRE(piranha::is_exponentiable_v<int &&, const double>);
    REQUIRE(!piranha::is_exponentiable_v<int, int>);
    REQUIRE(!piranha::is_exponentiable_v<float, void>);
    REQUIRE(!piranha::is_exponentiable_v<void, float>);
    REQUIRE(!piranha::is_exponentiable_v<void, void>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::is_exponentiable_v<float, __int128_t>);
    REQUIRE(piranha::is_exponentiable_v<float, __uint128_t>);
    REQUIRE(piranha::is_exponentiable_v<__int128_t &&, const double>);
    REQUIRE(piranha::is_exponentiable_v<__uint128_t &&, const double>);
    REQUIRE(!piranha::is_exponentiable_v<__uint128_t &&, __int128_t>);
    REQUIRE(!piranha::is_exponentiable_v<__int128_t &&, __uint128_t>);
#endif
#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::Exponentiable<float, int>);
    REQUIRE(piranha::Exponentiable<const double &, float &&>);
    REQUIRE(piranha::Exponentiable<long double &, float &&>);
    REQUIRE(!piranha::Exponentiable<int, const bool &>);
    REQUIRE(!piranha::Exponentiable<short, unsigned char &&>);
    REQUIRE(!piranha::Exponentiable<double, void>);
    REQUIRE(!piranha::Exponentiable<void, double>);
    REQUIRE(!piranha::Exponentiable<void, void>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::Exponentiable<float, __int128_t>);
    REQUIRE(piranha::Exponentiable<__uint128_t, long double &>);
    REQUIRE(!piranha::Exponentiable<int, const __int128_t>);
    REQUIRE(!piranha::Exponentiable<__uint128_t, const __int128_t>);
#endif
#endif

    REQUIRE(piranha::pow(3., 5) == std::pow(3., 5));
    REQUIRE(piranha::pow(5, 3.) == std::pow(5, 3.));
    REQUIRE(piranha::pow(3., -2.) == std::pow(3., -2.));
    REQUIRE(piranha::pow(3.f, -2.f) == std::pow(3.f, -2.f));
    REQUIRE(piranha::pow(3.l, -2.l) == std::pow(3.l, -2.l));
    REQUIRE(std::is_same_v<decltype(piranha::pow(3., 5)), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(5, 3.)), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.f, 5)), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(5, 3.f)), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.l, 5)), long double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(5, 3.l)), long double>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::pow(3., __int128_t(5)) == std::pow(3., 5));
    REQUIRE(piranha::pow(__uint128_t(5), 3.) == std::pow(5, 3.));
    REQUIRE(std::is_same_v<decltype(piranha::pow(3., __int128_t(5))), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3., __uint128_t(5))), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__int128_t(5), 3.)), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__uint128_t(5), 3.)), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.f, __int128_t(5))), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.f, __uint128_t(5))), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__int128_t(5), 3.f)), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__uint128_t(5), 3.f)), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.l, __int128_t(5))), long double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.l, __uint128_t(5))), long double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__int128_t(5), 3.l)), long double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__uint128_t(5), 3.l)), long double>);
#endif
}

TEST_CASE("pow_mp++_int")
{
    using int_t = mppp::integer<1>;

    REQUIRE(piranha::pow(int_t{3}, 5) == 243);
}
