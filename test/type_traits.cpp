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

#include <memory>
#include <string>
#include <thread>

#include <piranha/config.hpp>

using namespace piranha;

TEST_CASE("is_integral")
{
    REQUIRE(is_integral_v<int>);
    REQUIRE(is_integral_v<const int>);
    REQUIRE(is_integral_v<const volatile int>);
    REQUIRE(is_integral_v<volatile int>);
    REQUIRE(!is_integral_v<int &>);
    REQUIRE(!is_integral_v<const int &>);
    REQUIRE(!is_integral_v<int &&>);

    REQUIRE(is_integral_v<long>);
    REQUIRE(is_integral_v<const long>);
    REQUIRE(is_integral_v<const volatile long>);
    REQUIRE(is_integral_v<volatile long>);
    REQUIRE(!is_integral_v<long &>);
    REQUIRE(!is_integral_v<const long &>);
    REQUIRE(!is_integral_v<long &&>);

    REQUIRE(!is_integral_v<float>);
    REQUIRE(!is_integral_v<const double>);
    REQUIRE(!is_integral_v<const volatile long double>);
    REQUIRE(!is_integral_v<volatile float>);

    REQUIRE(!is_integral_v<std::string>);
    REQUIRE(!is_integral_v<void>);

#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(is_integral_v<__int128_t>);
    REQUIRE(is_integral_v<const __int128_t>);
    REQUIRE(is_integral_v<const volatile __int128_t>);
    REQUIRE(is_integral_v<volatile __int128_t>);
    REQUIRE(!is_integral_v<__int128_t &>);
    REQUIRE(!is_integral_v<const __int128_t &>);
    REQUIRE(!is_integral_v<__int128_t &&>);

    REQUIRE(is_integral_v<__uint128_t>);
    REQUIRE(is_integral_v<const __uint128_t>);
    REQUIRE(is_integral_v<const volatile __uint128_t>);
    REQUIRE(is_integral_v<volatile __uint128_t>);
    REQUIRE(!is_integral_v<__uint128_t &>);
    REQUIRE(!is_integral_v<const __uint128_t &>);
    REQUIRE(!is_integral_v<__uint128_t &&>);
#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(Integral<int>);
    REQUIRE(Integral<const int>);
    REQUIRE(Integral<const volatile int>);
    REQUIRE(Integral<volatile int>);
    REQUIRE(!Integral<int &>);
    REQUIRE(!Integral<const int &>);
    REQUIRE(!Integral<int &&>);
#endif
}

#if defined(PIRANHA_HAVE_CONCEPTS)
TEST_CASE("is_floating_point")
{
    REQUIRE(FloatingPoint<float>);
    REQUIRE(FloatingPoint<const float>);
    REQUIRE(FloatingPoint<const volatile float>);
    REQUIRE(FloatingPoint<volatile float>);
    REQUIRE(!FloatingPoint<float &>);
    REQUIRE(!FloatingPoint<const float &>);
    REQUIRE(!FloatingPoint<float &&>);
}
#endif

TEST_CASE("is_arithmetic")
{
    REQUIRE(is_arithmetic_v<int>);
    REQUIRE(is_arithmetic_v<const bool>);
    REQUIRE(is_arithmetic_v<const volatile float>);
    REQUIRE(is_arithmetic_v<volatile double>);
    REQUIRE(!is_arithmetic_v<float &>);
    REQUIRE(!is_arithmetic_v<const float &>);
    REQUIRE(!is_arithmetic_v<float &&>);

    REQUIRE(!is_arithmetic_v<std::string>);
    REQUIRE(!is_arithmetic_v<void>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(Arithmetic<float>);
    REQUIRE(Arithmetic<const bool>);
    REQUIRE(Arithmetic<const volatile long>);
    REQUIRE(Arithmetic<volatile char>);
    REQUIRE(!Arithmetic<float &>);
    REQUIRE(!Arithmetic<const int &>);
    REQUIRE(!Arithmetic<short &&>);
#endif
}

#if defined(PIRANHA_HAVE_CONCEPTS)
TEST_CASE("is_const")
{
    REQUIRE(!Const<void>);
    REQUIRE(Const<void const>);
    REQUIRE(Const<void const volatile>);
    REQUIRE(!Const<std::string>);
    REQUIRE(Const<std::string const>);
    REQUIRE(!Const<std::string &>);
    REQUIRE(!Const<const std::string &>);
}
#endif

TEST_CASE("is_signed")
{
    REQUIRE(!is_signed_v<void>);
    REQUIRE(!is_signed_v<unsigned>);
    REQUIRE(!is_signed_v<const unsigned>);
    REQUIRE(!is_signed_v<volatile unsigned>);
    REQUIRE(!is_signed_v<const volatile unsigned>);
    REQUIRE(is_signed_v<int>);
    REQUIRE(is_signed_v<const int>);
    REQUIRE(is_signed_v<volatile int>);
    REQUIRE(is_signed_v<const volatile int>);
    REQUIRE(!is_signed_v<int &>);
    REQUIRE(!is_signed_v<int &&>);
    REQUIRE(!is_signed_v<const int &>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(is_signed_v<__int128_t>);
    REQUIRE(is_signed_v<const __int128_t>);
    REQUIRE(is_signed_v<volatile __int128_t>);
    REQUIRE(is_signed_v<const volatile __int128_t>);
    REQUIRE(!is_signed_v<__int128_t &>);
    REQUIRE(!is_signed_v<__int128_t &&>);
    REQUIRE(!is_signed_v<const __int128_t &>);
    REQUIRE(!is_signed_v<__uint128_t>);
    REQUIRE(!is_signed_v<const __uint128_t>);
    REQUIRE(!is_signed_v<volatile __uint128_t>);
    REQUIRE(!is_signed_v<const volatile __uint128_t>);
    REQUIRE(!is_signed_v<__uint128_t &>);
    REQUIRE(!is_signed_v<__uint128_t &&>);
    REQUIRE(!is_signed_v<const __uint128_t &>);
#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!Signed<void>);
    REQUIRE(!Signed<void>);
    REQUIRE(!Signed<unsigned>);
    REQUIRE(!Signed<const unsigned>);
    REQUIRE(!Signed<volatile unsigned>);
    REQUIRE(!Signed<const volatile unsigned>);
    REQUIRE(Signed<int>);
    REQUIRE(Signed<const int>);
    REQUIRE(Signed<volatile int>);
    REQUIRE(Signed<const volatile int>);
    REQUIRE(!Signed<int &>);
    REQUIRE(!Signed<int &&>);
    REQUIRE(!Signed<const int &>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(Signed<__int128_t>);
    REQUIRE(Signed<const __int128_t>);
    REQUIRE(Signed<volatile __int128_t>);
    REQUIRE(Signed<const volatile __int128_t>);
    REQUIRE(!Signed<__int128_t &>);
    REQUIRE(!Signed<__int128_t &&>);
    REQUIRE(!Signed<const __int128_t &>);
    REQUIRE(!Signed<__uint128_t>);
    REQUIRE(!Signed<const __uint128_t>);
    REQUIRE(!Signed<volatile __uint128_t>);
    REQUIRE(!Signed<const volatile __uint128_t>);
    REQUIRE(!Signed<__uint128_t &>);
    REQUIRE(!Signed<__uint128_t &&>);
    REQUIRE(!Signed<const __uint128_t &>);
#endif
#endif
}

TEST_CASE("make_unsigned")
{
    REQUIRE((std::is_same_v<unsigned, make_unsigned_t<unsigned>>));
    REQUIRE((std::is_same_v<unsigned, make_unsigned_t<int>>));
    REQUIRE((std::is_same_v<const unsigned, make_unsigned_t<const int>>));
    REQUIRE((std::is_same_v<volatile unsigned, make_unsigned_t<volatile int>>));
    REQUIRE((std::is_same_v<const volatile unsigned, make_unsigned_t<const volatile int>>));
    REQUIRE((std::is_same_v<unsigned, make_unsigned_t<unsigned>>));
    REQUIRE((std::is_same_v<const unsigned, make_unsigned_t<const unsigned>>));
    REQUIRE((std::is_same_v<volatile unsigned, make_unsigned_t<volatile unsigned>>));
    REQUIRE((std::is_same_v<const volatile unsigned, make_unsigned_t<const volatile unsigned>>));
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE((std::is_same_v<__uint128_t, make_unsigned_t<__uint128_t>>));
    REQUIRE((std::is_same_v<const __uint128_t, make_unsigned_t<const __uint128_t>>));
    REQUIRE((std::is_same_v<volatile __uint128_t, make_unsigned_t<volatile __uint128_t>>));
    REQUIRE((std::is_same_v<const volatile __uint128_t, make_unsigned_t<const volatile __uint128_t>>));
    REQUIRE((std::is_same_v<__uint128_t, make_unsigned_t<__int128_t>>));
    REQUIRE((std::is_same_v<const __uint128_t, make_unsigned_t<const __int128_t>>));
    REQUIRE((std::is_same_v<volatile __uint128_t, make_unsigned_t<volatile __int128_t>>));
    REQUIRE((std::is_same_v<const volatile __uint128_t, make_unsigned_t<const volatile __int128_t>>));
#endif
}

struct unreturnable_00 {
    unreturnable_00(const unreturnable_00 &) = delete;
    unreturnable_00(unreturnable_00 &&) = delete;
};

struct unreturnable_01 {
    ~unreturnable_01() = delete;
};

TEST_CASE("is_returnable")
{
    REQUIRE(is_returnable_v<void>);
    REQUIRE(is_returnable_v<const void>);
    REQUIRE(is_returnable_v<volatile void>);
    REQUIRE(is_returnable_v<const volatile void>);
    REQUIRE(is_returnable_v<int>);
    REQUIRE(is_returnable_v<int &>);
    REQUIRE(is_returnable_v<const int &>);
    REQUIRE(is_returnable_v<int &&>);
    REQUIRE(is_returnable_v<int *>);
    REQUIRE(is_returnable_v<std::string>);
    REQUIRE(is_returnable_v<std::thread>);
    REQUIRE(is_returnable_v<std::unique_ptr<int>>);
    REQUIRE(is_returnable_v<std::shared_ptr<int>>);
    REQUIRE(!is_returnable_v<unreturnable_00>);
    REQUIRE(is_returnable_v<unreturnable_00 &>);
    REQUIRE(!is_returnable_v<unreturnable_01>);
    REQUIRE(is_returnable_v<unreturnable_01 &>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(Returnable<const volatile void>);
    REQUIRE(!Returnable<unreturnable_00>);
#endif
}

#if defined(PIRANHA_HAVE_CONCEPTS)
TEST_CASE("default_constructible")
{
    REQUIRE(DefaultConstructible<int>);
    REQUIRE(DefaultConstructible<int *>);
    REQUIRE(!DefaultConstructible<void>);
    REQUIRE(!DefaultConstructible<const void>);
    REQUIRE(!DefaultConstructible<int &>);
    REQUIRE(!DefaultConstructible<const int &>);
    REQUIRE(!DefaultConstructible<int &&>);
}
#endif
