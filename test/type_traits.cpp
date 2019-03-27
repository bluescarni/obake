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
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>

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

#if defined(PIRANHA_HAVE_CONCEPTS)
TEST_CASE("same")
{
    REQUIRE(Same<int, int>);
    REQUIRE(Same<void, void>);
    REQUIRE(!Same<void, const void>);
}
#endif

TEST_CASE("is_same_cvref")
{
    REQUIRE(is_same_cvref_v<int, int>);
    REQUIRE(is_same_cvref_v<int, int &>);
    REQUIRE(is_same_cvref_v<volatile int, int &>);
    REQUIRE(is_same_cvref_v<int &&, int &>);
    REQUIRE(is_same_cvref_v<const int &&, const int>);
    REQUIRE(is_same_cvref_v<int &, const int &>);
    REQUIRE(!is_same_cvref_v<void, int>);
    REQUIRE(!is_same_cvref_v<int, int *>);
    REQUIRE(!is_same_cvref_v<int *, int>);
    REQUIRE(is_same_cvref_v<void, void>);
    REQUIRE(is_same_cvref_v<void, const void>);
    REQUIRE(is_same_cvref_v<volatile void, const void>);
    REQUIRE(is_same_cvref_v<const volatile void, const void>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(SameCvref<int, int>);
    REQUIRE(SameCvref<int, int &>);
    REQUIRE(SameCvref<const int, int &>);
    REQUIRE(!SameCvref<int, void>);
    REQUIRE(!SameCvref<int *, int &>);
#endif
}

template <typename T, std::enable_if_t<is_string_like_v<T>, int> = 0>
void check_string_like_dispatch(const T &s)
{
    std::ostringstream o;
    o << s;
    REQUIRE(o.str() == s);
}

TEST_CASE("is_string_like_v")
{
    REQUIRE(!is_string_like_v<void>);
    REQUIRE(is_string_like_v<char *>);
    REQUIRE(is_string_like_v<const char *>);
    REQUIRE(is_string_like_v<char *const>);
    REQUIRE(is_string_like_v<const char *>);
    REQUIRE(is_string_like_v<const char *const>);
    REQUIRE(!is_string_like_v<char *&>);
    REQUIRE(is_string_like_v<char[]>);
    REQUIRE(is_string_like_v<const char[]>);
    REQUIRE(is_string_like_v<char[1]>);
    REQUIRE(is_string_like_v<const char[1]>);
    REQUIRE(is_string_like_v<char[2]>);
    REQUIRE(is_string_like_v<char[10]>);
    REQUIRE(is_string_like_v<const char[10]>);
    REQUIRE(!is_string_like_v<const char(&)[10]>);
    REQUIRE(!is_string_like_v<char(&)[10]>);
    REQUIRE(!is_string_like_v<char>);
    REQUIRE(!is_string_like_v<const char>);
    REQUIRE(!is_string_like_v<int>);
    REQUIRE(is_string_like_v<std::string>);
    REQUIRE(!is_string_like_v<std::string &>);
    REQUIRE(!is_string_like_v<const std::string &>);
    REQUIRE(is_string_like_v<const std::string>);
    REQUIRE(!is_string_like_v<char(&)[]>);
    REQUIRE(!is_string_like_v<char(&)[1]>);
    REQUIRE(is_string_like_v<const char[2]>);
    REQUIRE(!is_string_like_v<char(&&)[10]>);
    REQUIRE(is_string_like_v<std::string_view>);
    REQUIRE(!is_string_like_v<std::string_view &>);
    REQUIRE(!is_string_like_v<const std::string_view &>);
    REQUIRE(is_string_like_v<const std::string_view>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!StringLike<void>);
    REQUIRE(StringLike<char *>);
    REQUIRE(StringLike<const char *>);
    REQUIRE(!StringLike<char *&>);
    REQUIRE(!StringLike<const char(&)[10]>);
    REQUIRE(StringLike<std::string>);
    REQUIRE(StringLike<std::string_view>);
    REQUIRE(!StringLike<std::string &>);
#endif

    std::string s{"foo"};
    check_string_like_dispatch(std::string{"foo"});
    check_string_like_dispatch(s);
    check_string_like_dispatch(static_cast<const std::string &>(s));
    const char s1[] = "blab";
    check_string_like_dispatch(s1);
    check_string_like_dispatch(&s1[0]);
    check_string_like_dispatch("blab");
    char s2[] = "blab";
    check_string_like_dispatch(s2);
    check_string_like_dispatch(&s2[0]);
    const std::string_view sv1{"bubbbbba"};
    check_string_like_dispatch(sv1);
    std::string_view sv2{"bubbbba"};
    check_string_like_dispatch(sv2);
}

struct nonaddable_0 {
};

struct addable_0 {
    friend addable_0 operator+(addable_0, addable_0);
};

struct addable_1 {
    friend addable_1 operator+(addable_1, addable_0);
    friend addable_1 operator+(addable_0, addable_1);
};

struct nonaddable_1 {
    friend nonaddable_1 operator+(nonaddable_1, addable_0);
};

struct nonaddable_2 {
    friend nonaddable_2 operator+(nonaddable_2, addable_0);
    friend nonaddable_1 operator+(addable_0, nonaddable_2);
};

TEST_CASE("is_addable")
{
    REQUIRE(!is_addable_v<void>);
    REQUIRE(!is_addable_v<void, void>);
    REQUIRE(!is_addable_v<void, int>);
    REQUIRE(!is_addable_v<int, void>);
    REQUIRE(is_addable_v<int>);
    REQUIRE(is_addable_v<int, int>);
    REQUIRE(is_addable_v<const int, int &>);
    REQUIRE(is_addable_v<int &&, volatile int &>);
    REQUIRE(is_addable_v<std::string, char *>);
    REQUIRE(is_addable_v<std::string, char *>);
    REQUIRE(!is_addable_v<std::string, int>);
    REQUIRE(!is_addable_v<nonaddable_0>);
    REQUIRE(is_addable_v<addable_0>);
    REQUIRE(is_addable_v<addable_1, addable_0>);
    REQUIRE(!is_addable_v<nonaddable_1, addable_0>);
    REQUIRE(!is_addable_v<nonaddable_2, addable_0>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!Addable<void>);
    REQUIRE(!Addable<void, void>);
    REQUIRE(!Addable<void, int>);
    REQUIRE(!Addable<int, void>);
    REQUIRE(Addable<int>);
    REQUIRE(Addable<int, int>);
    REQUIRE(Addable<const int, int &>);
    REQUIRE(Addable<int &&, volatile int &>);
    REQUIRE(Addable<std::string, char *>);
    REQUIRE(Addable<std::string, char *>);
    REQUIRE(!Addable<std::string, int>);
    REQUIRE(!Addable<nonaddable_0>);
    REQUIRE(Addable<addable_0>);
    REQUIRE(Addable<addable_1, addable_0>);
    REQUIRE(!Addable<nonaddable_1, addable_0>);
    REQUIRE(!Addable<nonaddable_2, addable_0>);
#endif
}

struct noncomp_0 {
};

struct noncomp_1 {
    friend void operator==(const noncomp_1 &, const noncomp_1 &) {}
    friend void operator!=(const noncomp_1 &, const noncomp_1 &) {}
};

struct noncomp_2 {
    friend int operator==(const noncomp_2 &, const noncomp_2 &)
    {
        return 1;
    }
};

struct comp_0 {
    friend int operator==(const comp_0 &, const comp_0 &)
    {
        return 1;
    }
    friend int operator!=(const comp_0 &, const comp_0 &)
    {
        return 0;
    }
    friend int operator==(const comp_0 &, const noncomp_0 &)
    {
        return 1;
    }
    friend int operator!=(const comp_0 &, const noncomp_0 &)
    {
        return 0;
    }
};

TEST_CASE("is_equality_comparable")
{
    REQUIRE(!is_equality_comparable_v<void>);
    REQUIRE(!is_equality_comparable_v<void, void>);
    REQUIRE(!is_equality_comparable_v<int, void>);
    REQUIRE(!is_equality_comparable_v<void, int>);
    REQUIRE(is_equality_comparable_v<int>);
    REQUIRE(is_equality_comparable_v<int, int>);
    REQUIRE(is_equality_comparable_v<int, long>);
    REQUIRE(is_equality_comparable_v<std::string, std::string>);
    REQUIRE(!is_equality_comparable_v<std::string, int>);
    REQUIRE(!is_equality_comparable_v<int, std::string>);
    REQUIRE(!is_equality_comparable_v<noncomp_0>);
    REQUIRE(!is_equality_comparable_v<noncomp_1>);
    REQUIRE(!is_equality_comparable_v<noncomp_2>);
    REQUIRE(is_equality_comparable_v<comp_0>);
    REQUIRE(!is_equality_comparable_v<comp_0, noncomp_0>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!EqualityComparable<void>);
    REQUIRE(!EqualityComparable<void, void>);
    REQUIRE(!EqualityComparable<int, void>);
    REQUIRE(!EqualityComparable<void, int>);
    REQUIRE(EqualityComparable<int>);
    REQUIRE(EqualityComparable<int, int>);
    REQUIRE(EqualityComparable<int, long>);
    REQUIRE(EqualityComparable<std::string, std::string>);
    REQUIRE(!EqualityComparable<std::string, int>);
    REQUIRE(!EqualityComparable<int, std::string>);
    REQUIRE(!EqualityComparable<noncomp_0>);
    REQUIRE(!EqualityComparable<noncomp_1>);
    REQUIRE(!EqualityComparable<noncomp_2>);
    REQUIRE(EqualityComparable<comp_0>);
    REQUIRE(!EqualityComparable<comp_0, noncomp_0>);
#endif
}

TEST_CASE("is_pre_incrementable")
{
    REQUIRE(!is_pre_incrementable_v<void>);
    REQUIRE(is_pre_incrementable_v<int &>);
    REQUIRE(!is_pre_incrementable_v<const int &>);
    REQUIRE(!is_pre_incrementable_v<int &&>);
    REQUIRE(!is_pre_incrementable_v<std::string &>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!PreIncrementable<void>);
    REQUIRE(PreIncrementable<int &>);
    REQUIRE(!PreIncrementable<const int &>);
    REQUIRE(!PreIncrementable<int &&>);
    REQUIRE(!PreIncrementable<std::string &>);
#endif
}
