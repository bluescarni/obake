// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/type_traits.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <string>

#include <piranha/config.hpp>

using namespace piranha;

TEST_CASE("is_cpp_integral")
{
    REQUIRE(is_cpp_integral<int>);
    REQUIRE(is_cpp_integral<const int>);
    REQUIRE(is_cpp_integral<const volatile int>);
    REQUIRE(is_cpp_integral<volatile int>);
    REQUIRE(!is_cpp_integral<int &>);
    REQUIRE(!is_cpp_integral<const int &>);
    REQUIRE(!is_cpp_integral<int &&>);

    REQUIRE(is_cpp_integral<long>);
    REQUIRE(is_cpp_integral<const long>);
    REQUIRE(is_cpp_integral<const volatile long>);
    REQUIRE(is_cpp_integral<volatile long>);
    REQUIRE(!is_cpp_integral<long &>);
    REQUIRE(!is_cpp_integral<const long &>);
    REQUIRE(!is_cpp_integral<long &&>);

    REQUIRE(!is_cpp_integral<float>);
    REQUIRE(!is_cpp_integral<const double>);
    REQUIRE(!is_cpp_integral<const volatile long double>);
    REQUIRE(!is_cpp_integral<volatile float>);

    REQUIRE(!is_cpp_integral<std::string>);

#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(is_cpp_integral<__int128_t>);
    REQUIRE(is_cpp_integral<const __int128_t>);
    REQUIRE(is_cpp_integral<const volatile __int128_t>);
    REQUIRE(is_cpp_integral<volatile __int128_t>);
    REQUIRE(!is_cpp_integral<__int128_t &>);
    REQUIRE(!is_cpp_integral<const __int128_t &>);
    REQUIRE(!is_cpp_integral<__int128_t &&>);

    REQUIRE(is_cpp_integral<__uint128_t>);
    REQUIRE(is_cpp_integral<const __uint128_t>);
    REQUIRE(is_cpp_integral<const volatile __uint128_t>);
    REQUIRE(is_cpp_integral<volatile __uint128_t>);
    REQUIRE(!is_cpp_integral<__uint128_t &>);
    REQUIRE(!is_cpp_integral<const __uint128_t &>);
    REQUIRE(!is_cpp_integral<__uint128_t &&>);
#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(CppIntegral<int>);
    REQUIRE(CppIntegral<const int>);
    REQUIRE(CppIntegral<const volatile int>);
    REQUIRE(CppIntegral<volatile int>);
    REQUIRE(!CppIntegral<int &>);
    REQUIRE(!CppIntegral<const int &>);
    REQUIRE(!CppIntegral<int &&>);
#endif
}

TEST_CASE("is_cpp_floating_point")
{
    REQUIRE(is_cpp_floating_point<float>);
    REQUIRE(is_cpp_floating_point<const float>);
    REQUIRE(is_cpp_floating_point<const volatile float>);
    REQUIRE(is_cpp_floating_point<volatile float>);
    REQUIRE(!is_cpp_floating_point<float &>);
    REQUIRE(!is_cpp_floating_point<const float &>);
    REQUIRE(!is_cpp_floating_point<float &&>);

    REQUIRE(is_cpp_floating_point<double>);
    REQUIRE(is_cpp_floating_point<const double>);
    REQUIRE(is_cpp_floating_point<const volatile double>);
    REQUIRE(is_cpp_floating_point<volatile double>);
    REQUIRE(!is_cpp_floating_point<double &>);
    REQUIRE(!is_cpp_floating_point<const double &>);
    REQUIRE(!is_cpp_floating_point<double &&>);

    REQUIRE(!is_cpp_floating_point<int>);
    REQUIRE(!is_cpp_floating_point<const long>);
    REQUIRE(!is_cpp_floating_point<const volatile long long>);
    REQUIRE(!is_cpp_floating_point<volatile short>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(CppFloatingPoint<float>);
    REQUIRE(CppFloatingPoint<const float>);
    REQUIRE(CppFloatingPoint<const volatile float>);
    REQUIRE(CppFloatingPoint<volatile float>);
    REQUIRE(!CppFloatingPoint<float &>);
    REQUIRE(!CppFloatingPoint<const float &>);
    REQUIRE(!CppFloatingPoint<float &&>);
#endif
}
