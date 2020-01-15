// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <obake/config.hpp>

#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#if defined(OBAKE_HAVE_STRING_VIEW)

#include <string_view>

#endif

#include <obake/detail/limits.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

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

#if defined(OBAKE_HAVE_GCC_INT128)
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

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(Integral<int>);
    REQUIRE(Integral<const int>);
    REQUIRE(Integral<const volatile int>);
    REQUIRE(Integral<volatile int>);
    REQUIRE(!Integral<int &>);
    REQUIRE(!Integral<const int &>);
    REQUIRE(!Integral<int &&>);
#endif
}

#if defined(OBAKE_HAVE_CONCEPTS)
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

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(Arithmetic<float>);
    REQUIRE(Arithmetic<const bool>);
    REQUIRE(Arithmetic<const volatile long>);
    REQUIRE(Arithmetic<volatile char>);
    REQUIRE(!Arithmetic<float &>);
    REQUIRE(!Arithmetic<const int &>);
    REQUIRE(!Arithmetic<short &&>);
#endif
}

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
#if defined(OBAKE_HAVE_GCC_INT128)
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

#if defined(OBAKE_HAVE_CONCEPTS)
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
#if defined(OBAKE_HAVE_GCC_INT128)
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
#if defined(OBAKE_HAVE_GCC_INT128)
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

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(Returnable<const volatile void>);
    REQUIRE(!Returnable<unreturnable_00>);
#endif
}

TEST_CASE("is_same_cvr")
{
    REQUIRE(is_same_cvr_v<int, int>);
    REQUIRE(is_same_cvr_v<int, int &>);
    REQUIRE(is_same_cvr_v<volatile int, int &>);
    REQUIRE(is_same_cvr_v<int &&, int &>);
    REQUIRE(is_same_cvr_v<const int &&, const int>);
    REQUIRE(is_same_cvr_v<int &, const int &>);
    REQUIRE(!is_same_cvr_v<void, int>);
    REQUIRE(!is_same_cvr_v<int, int *>);
    REQUIRE(!is_same_cvr_v<int *, int>);
    REQUIRE(is_same_cvr_v<void, void>);
    REQUIRE(is_same_cvr_v<void, const void>);
    REQUIRE(is_same_cvr_v<volatile void, const void>);
    REQUIRE(is_same_cvr_v<const volatile void, const void>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(SameCvr<int, int>);
    REQUIRE(SameCvr<int, int &>);
    REQUIRE(SameCvr<const int, int &>);
    REQUIRE(!SameCvr<int, void>);
    REQUIRE(!SameCvr<int *, int &>);
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

#if defined(OBAKE_HAVE_STRING_VIEW)
    REQUIRE(is_string_like_v<std::string_view>);
    REQUIRE(!is_string_like_v<std::string_view &>);
    REQUIRE(!is_string_like_v<const std::string_view &>);
    REQUIRE(is_string_like_v<const std::string_view>);
#endif

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!StringLike<void>);
    REQUIRE(StringLike<char *>);
    REQUIRE(StringLike<const char *>);
    REQUIRE(!StringLike<char *&>);
    REQUIRE(!StringLike<const char(&)[10]>);
    REQUIRE(StringLike<std::string>);
#if defined(OBAKE_HAVE_STRING_VIEW)
    REQUIRE(StringLike<std::string_view>);
#endif
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
#if defined(OBAKE_HAVE_STRING_VIEW)
    const std::string_view sv1{"bubbbbba"};
    check_string_like_dispatch(sv1);
    std::string_view sv2{"bubbbba"};
    check_string_like_dispatch(sv2);
#endif
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
    REQUIRE(!is_addable_v<std::string, int>);
    REQUIRE(!is_addable_v<nonaddable_0>);
    REQUIRE(is_addable_v<addable_0>);
    REQUIRE(is_addable_v<addable_1, addable_0>);
    REQUIRE(!is_addable_v<nonaddable_1, addable_0>);
    REQUIRE(!is_addable_v<nonaddable_2, addable_0>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Addable<void>);
    REQUIRE(!Addable<void, void>);
    REQUIRE(!Addable<void, int>);
    REQUIRE(!Addable<int, void>);
    REQUIRE(Addable<int>);
    REQUIRE(Addable<int, int>);
    REQUIRE(Addable<const int, int &>);
    REQUIRE(Addable<int &&, volatile int &>);
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

#if defined(OBAKE_HAVE_CONCEPTS)
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

struct nonlt_0 {
};

struct nonlt_1 {
    friend void operator<(const nonlt_1 &, const nonlt_1 &) {}
};

struct lt_0 {
    friend int operator<(const lt_0 &, const lt_0 &)
    {
        return 1;
    }
    friend int operator<(const lt_0 &, const nonlt_0 &)
    {
        return 1;
    }
};

TEST_CASE("is_less_than_comparable")
{
    REQUIRE(!is_less_than_comparable_v<void>);
    REQUIRE(!is_less_than_comparable_v<void, void>);
    REQUIRE(!is_less_than_comparable_v<int, void>);
    REQUIRE(!is_less_than_comparable_v<void, int>);
    REQUIRE(is_less_than_comparable_v<int>);
    REQUIRE(is_less_than_comparable_v<int, int>);
    REQUIRE(is_less_than_comparable_v<int, long>);
    REQUIRE(is_less_than_comparable_v<std::string, std::string>);
    REQUIRE(!is_less_than_comparable_v<std::string, int>);
    REQUIRE(!is_less_than_comparable_v<int, std::string>);
    REQUIRE(!is_less_than_comparable_v<nonlt_0>);
    REQUIRE(!is_less_than_comparable_v<nonlt_1>);
    REQUIRE(is_less_than_comparable_v<lt_0>);
    REQUIRE(!is_less_than_comparable_v<lt_0, nonlt_0>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!LessThanComparable<void>);
    REQUIRE(!LessThanComparable<void, void>);
    REQUIRE(!LessThanComparable<int, void>);
    REQUIRE(!LessThanComparable<void, int>);
    REQUIRE(LessThanComparable<int>);
    REQUIRE(LessThanComparable<int, int>);
    REQUIRE(LessThanComparable<int, long>);
    REQUIRE(LessThanComparable<std::string, std::string>);
    REQUIRE(!LessThanComparable<std::string, int>);
    REQUIRE(!LessThanComparable<int, std::string>);
    REQUIRE(!LessThanComparable<nonlt_0>);
    REQUIRE(!LessThanComparable<nonlt_1>);
    REQUIRE(LessThanComparable<lt_0>);
    REQUIRE(!LessThanComparable<lt_0, nonlt_0>);
#endif
}

struct nongt_0 {
};

struct nongt_1 {
    friend void operator>(const nongt_1 &, const nongt_1 &) {}
};

struct gt_0 {
    friend int operator>(const gt_0 &, const gt_0 &)
    {
        return 1;
    }
    friend int operator>(const gt_0 &, const nongt_0 &)
    {
        return 1;
    }
};

TEST_CASE("is_greater_than_comparable")
{
    REQUIRE(!is_greater_than_comparable_v<void>);
    REQUIRE(!is_greater_than_comparable_v<void, void>);
    REQUIRE(!is_greater_than_comparable_v<int, void>);
    REQUIRE(!is_greater_than_comparable_v<void, int>);
    REQUIRE(is_greater_than_comparable_v<int>);
    REQUIRE(is_greater_than_comparable_v<int, int>);
    REQUIRE(is_greater_than_comparable_v<int, long>);
    REQUIRE(is_greater_than_comparable_v<std::string, std::string>);
    REQUIRE(!is_greater_than_comparable_v<std::string, int>);
    REQUIRE(!is_greater_than_comparable_v<int, std::string>);
    REQUIRE(!is_greater_than_comparable_v<nongt_0>);
    REQUIRE(!is_greater_than_comparable_v<nongt_1>);
    REQUIRE(is_greater_than_comparable_v<gt_0>);
    REQUIRE(!is_greater_than_comparable_v<gt_0, nongt_0>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!GreaterThanComparable<void>);
    REQUIRE(!GreaterThanComparable<void, void>);
    REQUIRE(!GreaterThanComparable<int, void>);
    REQUIRE(!GreaterThanComparable<void, int>);
    REQUIRE(GreaterThanComparable<int>);
    REQUIRE(GreaterThanComparable<int, int>);
    REQUIRE(GreaterThanComparable<int, long>);
    REQUIRE(GreaterThanComparable<std::string, std::string>);
    REQUIRE(!GreaterThanComparable<std::string, int>);
    REQUIRE(!GreaterThanComparable<int, std::string>);
    REQUIRE(!GreaterThanComparable<nongt_0>);
    REQUIRE(!GreaterThanComparable<nongt_1>);
    REQUIRE(GreaterThanComparable<gt_0>);
    REQUIRE(!GreaterThanComparable<gt_0, nongt_0>);
#endif
}

struct nonlte_0 {
};

struct nonlte_1 {
    friend void operator<=(const nonlte_1 &, const nonlte_1 &) {}
};

struct lte_0 {
    friend int operator<=(const lte_0 &, const lte_0 &)
    {
        return 1;
    }
    friend int operator<=(const lte_0 &, const nonlte_0 &)
    {
        return 1;
    }
};

TEST_CASE("is_lte_comparable")
{
    REQUIRE(!is_lte_comparable_v<void>);
    REQUIRE(!is_lte_comparable_v<void, void>);
    REQUIRE(!is_lte_comparable_v<int, void>);
    REQUIRE(!is_lte_comparable_v<void, int>);
    REQUIRE(is_lte_comparable_v<int>);
    REQUIRE(is_lte_comparable_v<int, int>);
    REQUIRE(is_lte_comparable_v<int, long>);
    REQUIRE(is_lte_comparable_v<std::string, std::string>);
    REQUIRE(!is_lte_comparable_v<std::string, int>);
    REQUIRE(!is_lte_comparable_v<int, std::string>);
    REQUIRE(!is_lte_comparable_v<nonlte_0>);
    REQUIRE(!is_lte_comparable_v<nonlte_1>);
    REQUIRE(is_lte_comparable_v<lte_0>);
    REQUIRE(!is_lte_comparable_v<lte_0, nonlte_0>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!LTEComparable<void>);
    REQUIRE(!LTEComparable<void, void>);
    REQUIRE(!LTEComparable<int, void>);
    REQUIRE(!LTEComparable<void, int>);
    REQUIRE(LTEComparable<int>);
    REQUIRE(LTEComparable<int, int>);
    REQUIRE(LTEComparable<int, long>);
    REQUIRE(LTEComparable<std::string, std::string>);
    REQUIRE(!LTEComparable<std::string, int>);
    REQUIRE(!LTEComparable<int, std::string>);
    REQUIRE(!LTEComparable<nonlte_0>);
    REQUIRE(!LTEComparable<nonlte_1>);
    REQUIRE(LTEComparable<lte_0>);
    REQUIRE(!LTEComparable<lte_0, nonlte_0>);
#endif
}

struct nongte_0 {
};

struct nongte_1 {
    friend void operator>=(const nongte_1 &, const nongte_1 &) {}
};

struct gte_0 {
    friend int operator>=(const gte_0 &, const gte_0 &)
    {
        return 1;
    }
    friend int operator>=(const gte_0 &, const nongte_0 &)
    {
        return 1;
    }
};

TEST_CASE("is_gte_comparable")
{
    REQUIRE(!is_gte_comparable_v<void>);
    REQUIRE(!is_gte_comparable_v<void, void>);
    REQUIRE(!is_gte_comparable_v<int, void>);
    REQUIRE(!is_gte_comparable_v<void, int>);
    REQUIRE(is_gte_comparable_v<int>);
    REQUIRE(is_gte_comparable_v<int, int>);
    REQUIRE(is_gte_comparable_v<int, long>);
    REQUIRE(is_gte_comparable_v<std::string, std::string>);
    REQUIRE(!is_gte_comparable_v<std::string, int>);
    REQUIRE(!is_gte_comparable_v<int, std::string>);
    REQUIRE(!is_gte_comparable_v<nongte_0>);
    REQUIRE(!is_gte_comparable_v<nongte_1>);
    REQUIRE(is_gte_comparable_v<gte_0>);
    REQUIRE(!is_gte_comparable_v<gte_0, nongte_0>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!GTEComparable<void>);
    REQUIRE(!GTEComparable<void, void>);
    REQUIRE(!GTEComparable<int, void>);
    REQUIRE(!GTEComparable<void, int>);
    REQUIRE(GTEComparable<int>);
    REQUIRE(GTEComparable<int, int>);
    REQUIRE(GTEComparable<int, long>);
    REQUIRE(GTEComparable<std::string, std::string>);
    REQUIRE(!GTEComparable<std::string, int>);
    REQUIRE(!GTEComparable<int, std::string>);
    REQUIRE(!GTEComparable<nongte_0>);
    REQUIRE(!GTEComparable<nongte_1>);
    REQUIRE(GTEComparable<gte_0>);
    REQUIRE(!GTEComparable<gte_0, nongte_0>);
#endif
}

TEST_CASE("is_pre_incrementable")
{
    REQUIRE(!is_pre_incrementable_v<void>);
    REQUIRE(is_pre_incrementable_v<int &>);
    REQUIRE(!is_pre_incrementable_v<const int &>);
    REQUIRE(!is_pre_incrementable_v<int &&>);
    REQUIRE(!is_pre_incrementable_v<std::string &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!PreIncrementable<void>);
    REQUIRE(PreIncrementable<int &>);
    REQUIRE(!PreIncrementable<const int &>);
    REQUIRE(!PreIncrementable<int &&>);
    REQUIRE(!PreIncrementable<std::string &>);
#endif
}

TEST_CASE("is_post_incrementable")
{
    REQUIRE(!is_post_incrementable_v<void>);
    REQUIRE(is_post_incrementable_v<int &>);
    REQUIRE(!is_post_incrementable_v<const int &>);
    REQUIRE(!is_post_incrementable_v<int &&>);
    REQUIRE(!is_post_incrementable_v<std::string &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!PostIncrementable<void>);
    REQUIRE(PostIncrementable<int &>);
    REQUIRE(!PostIncrementable<const int &>);
    REQUIRE(!PostIncrementable<int &&>);
    REQUIRE(!PostIncrementable<std::string &>);
#endif
}

TEST_CASE("is_pre_decrementable")
{
    REQUIRE(!is_pre_decrementable_v<void>);
    REQUIRE(is_pre_decrementable_v<int &>);
    REQUIRE(!is_pre_decrementable_v<const int &>);
    REQUIRE(!is_pre_decrementable_v<int &&>);
    REQUIRE(!is_pre_decrementable_v<std::string &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!PreDecrementable<void>);
    REQUIRE(PreDecrementable<int &>);
    REQUIRE(!PreDecrementable<const int &>);
    REQUIRE(!PreDecrementable<int &&>);
    REQUIRE(!PreDecrementable<std::string &>);
#endif
}

TEST_CASE("is_post_decrementable")
{
    REQUIRE(!is_post_decrementable_v<void>);
    REQUIRE(is_post_decrementable_v<int &>);
    REQUIRE(!is_post_decrementable_v<const int &>);
    REQUIRE(!is_post_decrementable_v<int &&>);
    REQUIRE(!is_post_decrementable_v<std::string &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!PostDecrementable<void>);
    REQUIRE(PostDecrementable<int &>);
    REQUIRE(!PostDecrementable<const int &>);
    REQUIRE(!PostDecrementable<int &&>);
    REQUIRE(!PostDecrementable<std::string &>);
#endif
}

// Boilerplate to test the arrow op type trait.
struct arrow01 {
    int *operator->();
};

struct arrow02 {
    arrow01 operator->();
    // NOTE: calling -> on a const instance will fail,
    // as it returns a non pointer type which does not
    // provide an operator->() member function.
    void operator->() const;
};

struct arrow03 {
    int operator->();
};

struct arrow03a {
    arrow02 operator->();
};

struct arrow04 {
    arrow03 operator->();
};

// Good forward iterator.
template <typename T>
struct fake_it_traits_forward {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::forward_iterator_tag;
};

// Broken reference type for forward it.
template <typename T>
struct fake_it_traits_forward_broken_ref {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = void;
    using iterator_category = std::forward_iterator_tag;
};

// Good output iterator.
template <typename T>
struct fake_it_traits_output {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::output_iterator_tag;
};

// Good input iterator.
template <typename T>
struct fake_it_traits_input {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::input_iterator_tag;
};

// Broken trait, incorrect category.
template <typename T>
struct fake_it_traits_wrong_tag {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = void;
};

// Broken trait, missing typedefs.
template <typename T>
struct fake_it_traits_missing {
    using value_type = void;
    using pointer = void;
    using iterator_category = void;
};

#define OBAKE_DECL_ITT_SPEC(it_type, trait_class)                                                                      \
    namespace std                                                                                                      \
    {                                                                                                                  \
    template <>                                                                                                        \
    struct iterator_traits<it_type> : trait_class {                                                                    \
    };                                                                                                                 \
    }

// Good input iterator.
struct iter01 {
    int &operator*() const;
    int *operator->() const;
    iter01 &operator++();
    iter01 &operator++(int);
    bool operator==(const iter01 &) const;
    bool operator!=(const iter01 &) const;
};

OBAKE_DECL_ITT_SPEC(iter01, fake_it_traits_input<int>)

// Good iterator, minimal requirements.
struct iter02 {
    int &operator*();
    iter02 &operator++();
};

OBAKE_DECL_ITT_SPEC(iter02, fake_it_traits_input<int>)

// Broken iterator, minimal requirements.
struct iter03 {
    // int &operator*();
    iter03 &operator++();
};

OBAKE_DECL_ITT_SPEC(iter03, fake_it_traits_input<int>)

// Broken iterator, minimal requirements.
struct iter04 {
    iter04 &operator=(const iter04 &) = delete;
    ~iter04() = delete;
    int &operator*();
    iter04 &operator++();
};

OBAKE_DECL_ITT_SPEC(iter04, fake_it_traits_input<int>)

// Broken iterator, missing itt spec.
struct iter05 {
    int &operator*();
    iter05 &operator++();
};

// OBAKE_DECL_ITT_SPEC(iter05,fake_it_traits_input<int>)

// Good input iterator: missing arrow, but the value type is not a class.
struct iter06 {
    int &operator*() const;
    // int *operator->();
    iter06 &operator++();
    iter06 &operator++(int);
    bool operator==(const iter06 &) const;
    bool operator!=(const iter06 &) const;
};

OBAKE_DECL_ITT_SPEC(iter06, fake_it_traits_input<int>)

struct iter06a_v {
};

// Bad input iterator: missing arrow, and the value type is a class.
struct iter06a {
    iter06a_v &operator*() const;
    // int *operator->();
    iter06a &operator++();
    iter06a &operator++(int);
    bool operator==(const iter06a &) const;
    bool operator!=(const iter06a &) const;
};

OBAKE_DECL_ITT_SPEC(iter06a, fake_it_traits_input<iter06a_v>)

// Broken input iterator: missing equality.
struct iter07 {
    int &operator*() const;
    int *operator->() const;
    iter07 &operator++();
    iter07 &operator++(int);
    // bool operator==(const iter07 &) const;
    bool operator!=(const iter07 &) const;
};

OBAKE_DECL_ITT_SPEC(iter07, fake_it_traits_input<int>)

// Broken input iterator: missing itt spec.
struct iter08 {
    int &operator*() const;
    int *operator->() const;
    iter08 &operator++();
    iter08 &operator++(int);
    bool operator==(const iter08 &) const;
    bool operator!=(const iter08 &) const;
};

// OBAKE_DECL_ITT_SPEC(iter08,fake_it_traits_input<int>)

// Good input iterator: broken arrow, but non-class.
struct iter09 {
    int &operator*() const;
    int operator->() const;
    iter09 &operator++();
    iter09 &operator++(int);
    bool operator==(const iter09 &) const;
    bool operator!=(const iter09 &) const;
};

OBAKE_DECL_ITT_SPEC(iter09, fake_it_traits_input<int>)

struct iter09a_v {
};

// Bad input iterator: broken arrow, and class value type.
struct iter09a {
    iter09a_v &operator*() const;
    iter09a_v operator->() const;
    iter09a &operator++();
    iter09a &operator++(int);
    bool operator==(const iter09a &) const;
    bool operator!=(const iter09a &) const;
};

OBAKE_DECL_ITT_SPEC(iter09a, fake_it_traits_input<iter09a_v>)

// Good input iterator: multiple arrow.
struct iter10 {
    int &operator*() const;
    arrow03a operator->() const;
    iter10 &operator++();
    iter10 &operator++(int);
    bool operator==(const iter10 &) const;
    bool operator!=(const iter10 &) const;
};

OBAKE_DECL_ITT_SPEC(iter10, fake_it_traits_input<int>)

// Good input iterator: multiple broken arrow, but non-class.
struct iter11 {
    int &operator*() const;
    arrow04 operator->() const;
    iter11 &operator++();
    iter11 &operator++(int);
    bool operator==(const iter11 &) const;
    bool operator!=(const iter11 &) const;
};

OBAKE_DECL_ITT_SPEC(iter11, fake_it_traits_input<int>)

// Bad input iterator: inconsistent arrow / star, and class value.
struct foo_it_12 {
};

struct iter12_v {
};

struct iter12 {
    iter12_v &operator*() const;
    foo_it_12 *operator->() const;
    iter12 &operator++();
    iter12 &operator++(int);
    bool operator==(const iter12 &) const;
    bool operator!=(const iter12 &) const;
};

OBAKE_DECL_ITT_SPEC(iter12, fake_it_traits_input<iter12_v>)

// Good input iterator: different but compatible arrow / star.
struct iter13 {
    int operator*() const;
    int *operator->() const;
    iter13 &operator++();
    iter13 &operator++(int);
    bool operator==(const iter13 &) const;
    bool operator!=(const iter13 &) const;
};

// Specialise the it_traits for iter13 manually, as we need
// a custom reference type.
namespace std
{
template <>
struct iterator_traits<iter13> {
    using difference_type = std::ptrdiff_t;
    using value_type = int;
    using pointer = int *;
    using reference = int;
    using iterator_category = std::input_iterator_tag;
};
} // namespace std

// Good forward iterator.
struct iter14 {
    int &operator*() const;
    int *operator->() const;
    iter14 &operator++();
    iter14 &operator++(int);
    bool operator==(const iter14 &) const;
    bool operator!=(const iter14 &) const;
};

OBAKE_DECL_ITT_SPEC(iter14, fake_it_traits_forward<int>)

// Bad forward iterator: missing def ctor.
struct iter15 {
    iter15() = delete;
    int &operator*() const;
    int *operator->() const;
    iter15 &operator++();
    iter15 &operator++(int);
    bool operator==(const iter15 &) const;
    bool operator!=(const iter15 &) const;
};

OBAKE_DECL_ITT_SPEC(iter15, fake_it_traits_forward<int>)

// Bad forward iterator: not having reference types as reference in traits.
struct iter16 {
    int &operator*() const;
    int *operator->() const;
    iter16 &operator++();
    iter16 &operator++(int);
    bool operator==(const iter16 &) const;
    bool operator!=(const iter16 &) const;
};

OBAKE_DECL_ITT_SPEC(iter16, fake_it_traits_forward_broken_ref<int>)

// Bad forward iterator: broken tag in traits.
struct iter17 {
    int &operator*() const;
    int *operator->() const;
    iter17 &operator++();
    iter17 &operator++(int);
    bool operator==(const iter17 &) const;
    bool operator!=(const iter17 &) const;
};

OBAKE_DECL_ITT_SPEC(iter17, fake_it_traits_output<int>)

// Bad forward iterator: broken traits.
struct iter18 {
    int &operator*() const;
    int *operator->() const;
    iter18 &operator++();
    iter18 &operator++(int);
    bool operator==(const iter18 &) const;
    bool operator!=(const iter18 &) const;
};

OBAKE_DECL_ITT_SPEC(iter18, fake_it_traits_missing<int>)

// Bad forward iterator: broken ++.
struct iter19 {
    int &operator*() const;
    int *operator->() const;
    iter19 &operator++();
    void operator++(int);
    bool operator==(const iter19 &) const;
    bool operator!=(const iter19 &) const;
};

OBAKE_DECL_ITT_SPEC(iter19, fake_it_traits_forward<int>)

// Bad forward iterator: broken ++.
struct iter20 {
    int &operator*() const;
    int *operator->() const;
    void operator++();
    iter20 &operator++(int);
    bool operator==(const iter20 &) const;
    bool operator!=(const iter20 &) const;
};

OBAKE_DECL_ITT_SPEC(iter20, fake_it_traits_forward<int>)

struct iter21_v {
};

// Bad forward iterator: arrow returns type with different constness from star operator,
// and class value.
struct iter21 {
    iter21_v &operator*() const;
    const iter21_v *operator->() const;
    iter21 &operator++();
    iter21 &operator++(int);
    bool operator==(const iter21 &) const;
    bool operator!=(const iter21 &) const;
};

OBAKE_DECL_ITT_SPEC(iter21, fake_it_traits_forward<iter21_v>)

#undef OBAKE_DECL_ITT_SPEC

TEST_CASE("iterators")
{
    // Check the arrow operator type trait.
    REQUIRE((!is_detected_v<detail::arrow_operator_t, void>));
    REQUIRE((std::is_same_v<int *, detected_t<detail::arrow_operator_t, int *&>>));
    REQUIRE((!is_detected_v<detail::arrow_operator_t, int &>));
    REQUIRE((std::is_same_v<int *, detected_t<detail::arrow_operator_t, arrow01 &>>));
    REQUIRE((std::is_same_v<int *, detected_t<detail::arrow_operator_t, arrow02 &>>));
    REQUIRE((!is_detected_v<detail::arrow_operator_t, const arrow02 &>));
    REQUIRE((!is_detected_v<detail::arrow_operator_t, arrow03 &>));
    REQUIRE((std::is_same_v<int *, detected_t<detail::arrow_operator_t, arrow03a &>>));

    // Iterator.
    REQUIRE(detail::has_iterator_traits<int *>::value);
    REQUIRE(detail::has_iterator_traits<const int *>::value);
    REQUIRE(!detail::has_iterator_traits<int>::value);
    REQUIRE(!detail::has_iterator_traits<double>::value);
    REQUIRE(detail::has_iterator_traits<std::vector<int>::iterator>::value);
    REQUIRE(detail::has_iterator_traits<std::vector<int>::const_iterator>::value);
    REQUIRE(!is_iterator_v<void>);
    REQUIRE(is_iterator_v<int *>);
    REQUIRE(is_iterator_v<const int *>);
    REQUIRE(is_iterator_v<std::vector<int>::iterator>);
    REQUIRE(is_iterator_v<std::vector<int>::const_iterator>);
    REQUIRE(!is_iterator_v<std::vector<int>::iterator &>);
    REQUIRE(!is_iterator_v<int>);
    REQUIRE(!is_iterator_v<std::string>);
    REQUIRE(is_iterator_v<iter01>);
    REQUIRE(!is_iterator_v<iter01 &>);
    REQUIRE(!is_iterator_v<const iter01>);
    REQUIRE(is_iterator_v<iter02>);
    REQUIRE(!is_iterator_v<iter02 &>);
    REQUIRE(!is_iterator_v<const iter02>);
    REQUIRE(!is_iterator_v<iter03>);
    REQUIRE(!is_iterator_v<iter03 &>);
    REQUIRE(!is_iterator_v<const iter03>);
    REQUIRE(is_iterator_v<std::istreambuf_iterator<char>>);
    REQUIRE(!is_iterator_v<iter04>);
    REQUIRE(!is_iterator_v<iter04 &>);
    REQUIRE(!is_iterator_v<const iter04>);
    REQUIRE(!is_iterator_v<iter05>);
    REQUIRE(!is_iterator_v<iter05 &>);
    REQUIRE(!is_iterator_v<const iter05>);
    REQUIRE(is_iterator_v<std::ostream_iterator<int>>);
    REQUIRE(is_iterator_v<std::insert_iterator<std::list<int>>>);

    // Input iterator.
    REQUIRE(!is_input_iterator_v<void>);
    REQUIRE(is_input_iterator_v<int *>);
    REQUIRE(is_input_iterator_v<const int *>);
    REQUIRE(is_input_iterator_v<std::vector<int>::iterator>);
    REQUIRE(is_input_iterator_v<std::vector<int>::const_iterator>);
    REQUIRE(!is_input_iterator_v<std::vector<int>::iterator &>);
    REQUIRE(is_input_iterator_v<std::istream_iterator<char>>);
    REQUIRE(is_input_iterator_v<std::istreambuf_iterator<char>>);
    REQUIRE(is_input_iterator_v<iter01>);
    REQUIRE((is_output_iterator_v<iter01, int &>));
    REQUIRE((!is_output_iterator_v<iter01, void>));
    REQUIRE(!is_input_iterator_v<iter01 &>);
    REQUIRE(!is_input_iterator_v<const iter01>);
    REQUIRE(!is_input_iterator_v<iter02>);
    REQUIRE(!is_input_iterator_v<iter02 &>);
    REQUIRE(!is_input_iterator_v<const iter02>);
    REQUIRE(is_input_iterator_v<iter06>);
    REQUIRE(!is_input_iterator_v<iter06 &>);
    REQUIRE(!is_input_iterator_v<const iter06>);
    REQUIRE(is_iterator_v<iter06>);
    REQUIRE(!is_iterator_v<iter06 &>);
    REQUIRE(!is_iterator_v<const iter06>);
    REQUIRE(!is_input_iterator_v<iter06a>);
    REQUIRE(!is_input_iterator_v<iter07>);
    REQUIRE(!is_input_iterator_v<iter07 &>);
    REQUIRE(!is_input_iterator_v<const iter07>);
    REQUIRE(is_iterator_v<iter07>);
    REQUIRE(!is_iterator_v<iter07 &>);
    REQUIRE(!is_iterator_v<const iter07>);
    REQUIRE(!is_input_iterator_v<iter08>);
    REQUIRE(!is_input_iterator_v<iter08 &>);
    REQUIRE(!is_input_iterator_v<const iter08>);

#if OBAKE_CPLUSPLUS > 201703L
    // C++20 dependent, different interfaces for iterators
    // (iterators don't need to provide explicit specialisations
    // of iterator_traits any more).
    REQUIRE(is_iterator_v<iter08>);
#else
    REQUIRE(!is_iterator_v<iter08>);
#endif

    REQUIRE(!is_iterator_v<iter08 &>);
    REQUIRE(!is_iterator_v<const iter08>);
    REQUIRE(is_input_iterator_v<iter09>);
    REQUIRE(!is_input_iterator_v<iter09 &>);
    REQUIRE(!is_input_iterator_v<const iter09>);
    REQUIRE(!is_input_iterator_v<iter09a>);
    REQUIRE(is_input_iterator_v<iter10>);
    REQUIRE((is_output_iterator_v<iter10, int &>));
    REQUIRE(!is_input_iterator_v<iter10 &>);
    REQUIRE(!is_input_iterator_v<const iter10>);
    REQUIRE(is_input_iterator_v<iter11>);
    REQUIRE(!is_input_iterator_v<iter11 &>);
    REQUIRE(!is_input_iterator_v<const iter11>);
    REQUIRE(is_iterator_v<iter11>);
    REQUIRE(!is_iterator_v<iter11 &>);
    REQUIRE(!is_iterator_v<const iter11>);
    REQUIRE(!is_input_iterator_v<iter12>);
    REQUIRE(!is_input_iterator_v<iter12 &>);
    REQUIRE(!is_input_iterator_v<const iter12>);
    REQUIRE(is_iterator_v<iter12>);
    REQUIRE(!is_iterator_v<iter12 &>);
    REQUIRE(!is_iterator_v<const iter12>);
    REQUIRE(is_input_iterator_v<iter13>);
    // NOTE: cannot use iter13 for writing, as it dereferences
    // to an int rather than int &.
    REQUIRE((!is_output_iterator_v<iter13, int &>));
    REQUIRE(!is_input_iterator_v<iter13 &>);
    REQUIRE(!is_input_iterator_v<const iter13>);

    // Forward iterator.
    REQUIRE(!is_forward_iterator_v<void>);
    REQUIRE(is_forward_iterator_v<int *>);
    REQUIRE((is_output_iterator_v<int *, int &>));
    REQUIRE(is_forward_iterator_v<const int *>);
    REQUIRE(is_forward_iterator_v<std::vector<int>::iterator>);
    REQUIRE((is_output_iterator_v<std::vector<int>::iterator, int &>));
    REQUIRE((is_output_iterator_v<std::vector<int>::iterator, double &>));
    REQUIRE((!is_output_iterator_v<std::vector<int>::iterator, std::string &>));
    REQUIRE(is_forward_iterator_v<std::vector<int>::const_iterator>);
    REQUIRE(!is_forward_iterator_v<std::vector<int>::iterator &>);
    REQUIRE(!is_forward_iterator_v<std::istream_iterator<char>>);
    REQUIRE((is_forward_iterator_v<std::map<int, int>::iterator>));
    REQUIRE(is_forward_iterator_v<iter14>);
    REQUIRE((is_output_iterator_v<iter14, int>));
    REQUIRE(!is_forward_iterator_v<iter14 &>);
    REQUIRE(!is_forward_iterator_v<const iter14>);
    REQUIRE(!is_forward_iterator_v<iter15>);
    REQUIRE(!is_forward_iterator_v<iter15 &>);
    REQUIRE(!is_forward_iterator_v<const iter15>);
    REQUIRE(is_input_iterator_v<iter15>);
    REQUIRE(!is_input_iterator_v<iter15 &>);
    REQUIRE(!is_input_iterator_v<const iter15>);
    REQUIRE(!is_forward_iterator_v<iter17>);
    REQUIRE(!is_forward_iterator_v<iter17 &>);
    REQUIRE(!is_forward_iterator_v<const iter17>);
    REQUIRE(is_iterator_v<iter17>);
    REQUIRE(!is_iterator_v<iter17 &>);
    REQUIRE(!is_iterator_v<const iter17>);
    REQUIRE(!is_forward_iterator_v<iter18>);
    REQUIRE(!is_forward_iterator_v<iter18 &>);
    REQUIRE(!is_forward_iterator_v<const iter18>);
    REQUIRE(!is_iterator_v<iter18>);
    REQUIRE(!is_iterator_v<iter18 &>);
    REQUIRE(!is_iterator_v<const iter18>);
    REQUIRE(!is_forward_iterator_v<iter19>);
    REQUIRE(!is_forward_iterator_v<iter19 &>);
    REQUIRE(!is_forward_iterator_v<const iter19>);
    REQUIRE(!is_input_iterator_v<iter19>);
    REQUIRE(!is_input_iterator_v<iter19 &>);
    REQUIRE(!is_input_iterator_v<const iter19>);
    REQUIRE(!is_forward_iterator_v<iter20>);
    REQUIRE(!is_forward_iterator_v<iter20 &>);
    REQUIRE(!is_forward_iterator_v<const iter20>);
    REQUIRE(!is_input_iterator_v<iter20>);
    REQUIRE(!is_input_iterator_v<iter20 &>);
    REQUIRE(!is_input_iterator_v<const iter20>);
    REQUIRE(!is_forward_iterator_v<iter21>);
    REQUIRE(!is_forward_iterator_v<iter21 &>);
    REQUIRE(!is_forward_iterator_v<const iter21>);
    REQUIRE(!is_input_iterator_v<iter21>);
    REQUIRE(!is_input_iterator_v<iter21 &>);
    REQUIRE(!is_input_iterator_v<const iter21>);
    REQUIRE(is_iterator_v<iter21>);
    REQUIRE(!is_iterator_v<iter21 &>);
    REQUIRE(!is_iterator_v<const iter21>);

    // Mutable forward iterator.
    REQUIRE((!is_mutable_forward_iterator_v<void>));
    REQUIRE((is_mutable_forward_iterator_v<int *>));
    REQUIRE((is_mutable_forward_iterator_v<std::vector<int>::iterator>));
    REQUIRE((is_mutable_forward_iterator_v<std::list<int>::iterator>));
    REQUIRE((!is_mutable_forward_iterator_v<const int *>));
    REQUIRE((!is_mutable_forward_iterator_v<std::vector<int>::const_iterator>));
    REQUIRE((!is_mutable_forward_iterator_v<std::istreambuf_iterator<char>>));
    REQUIRE((!is_mutable_forward_iterator_v<std::list<int>::const_iterator>));
    REQUIRE((!is_mutable_forward_iterator_v<std::set<int>::iterator>));
    REQUIRE((is_mutable_forward_iterator_v<std::map<int, int>::iterator>));
    REQUIRE((!is_mutable_forward_iterator_v<std::map<int, int>::const_iterator>));

    // Output iterator.
    REQUIRE((!is_output_iterator_v<void, void>));
    REQUIRE((!is_output_iterator_v<void, double>));
    REQUIRE((!is_output_iterator_v<double, void>));
    REQUIRE((is_output_iterator_v<std::ostream_iterator<double>, double &>));
    REQUIRE((is_output_iterator_v<std::ostream_iterator<double>, int>));
    REQUIRE((!is_output_iterator_v<std::ostream_iterator<double>, std::string &>));
    REQUIRE((!is_input_iterator_v<std::ostream_iterator<double>>));
    REQUIRE((is_output_iterator_v<int *, int &>));
    REQUIRE((is_output_iterator_v<int *, int &&>));
    REQUIRE((is_output_iterator_v<int *, double &&>));
    REQUIRE((!is_output_iterator_v<int *, std::string &>));
    REQUIRE((is_output_iterator_v<std::list<int>::iterator, int &>));
    REQUIRE((!is_output_iterator_v<std::list<int>::const_iterator, int &>));

    // Bidirectional iterator.
    REQUIRE(!is_bidirectional_iterator_v<void>);
    REQUIRE(!is_bidirectional_iterator_v<iter14>);
    REQUIRE(is_bidirectional_iterator_v<int *>);
    REQUIRE(is_bidirectional_iterator_v<const int *>);
    REQUIRE(is_bidirectional_iterator_v<std::vector<int>::iterator>);
    REQUIRE(is_bidirectional_iterator_v<std::vector<int>::const_iterator>);
    REQUIRE(!is_bidirectional_iterator_v<std::vector<int>::iterator &>);
    REQUIRE(!is_bidirectional_iterator_v<std::istream_iterator<char>>);

    // Random access iterator.
    REQUIRE(!is_random_access_iterator_v<void>);
    REQUIRE(!is_random_access_iterator_v<iter14>);
    REQUIRE(is_random_access_iterator_v<int *>);
    REQUIRE(is_random_access_iterator_v<const int *>);
    REQUIRE(is_random_access_iterator_v<std::vector<int>::iterator>);
    REQUIRE(is_random_access_iterator_v<std::vector<int>::const_iterator>);
    REQUIRE(!is_random_access_iterator_v<std::vector<int>::iterator &>);
    REQUIRE(!is_random_access_iterator_v<std::istream_iterator<char>>);
    REQUIRE((!is_random_access_iterator_v<std::list<int>::const_iterator>));
    REQUIRE((!is_random_access_iterator_v<std::map<int, int>::const_iterator>));
    REQUIRE((!is_random_access_iterator_v<std::unordered_map<int, int>::const_iterator>));

#if defined(OBAKE_HAVE_CONCEPTS)
    // Just a few concept checks, as currently the concepts
    // are based on the type traits.
    REQUIRE(!Iterator<void>);
    REQUIRE(Iterator<char *>);
    REQUIRE(!InputIterator<void>);
    REQUIRE(InputIterator<int *>);
    REQUIRE(!ForwardIterator<void>);
    REQUIRE(ForwardIterator<int *>);
    REQUIRE(!MutableForwardIterator<void>);
    REQUIRE(MutableForwardIterator<int *>);
    REQUIRE(!OutputIterator<void, void>);
    REQUIRE((!OutputIterator<void, double>));
    REQUIRE((!OutputIterator<double, void>));
    REQUIRE((OutputIterator<int *, int &>));
    REQUIRE((!OutputIterator<int *, std::string &>));
    REQUIRE(BidirectionalIterator<int *>);
    REQUIRE(!BidirectionalIterator<void>);
    REQUIRE(RandomAccessIterator<int *>);
    REQUIRE(!RandomAccessIterator<const void>);
    REQUIRE(!RandomAccessIterator<std::unordered_map<int, int>::const_iterator>);
#endif
}

TEST_CASE("limits_digits")
{
    REQUIRE(std::numeric_limits<int>::digits == detail::limits_digits<int>);
    REQUIRE(std::numeric_limits<unsigned>::digits == detail::limits_digits<unsigned>);
    REQUIRE(std::numeric_limits<signed char>::digits == detail::limits_digits<signed char>);
    REQUIRE(std::numeric_limits<unsigned short>::digits == detail::limits_digits<unsigned short>);
    REQUIRE(std::numeric_limits<long long>::digits == detail::limits_digits<long long>);
    REQUIRE(std::numeric_limits<float>::digits == detail::limits_digits<float>);
    REQUIRE(std::numeric_limits<double>::digits == detail::limits_digits<double>);

#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(128 == detail::limits_digits<__uint128_t>);
    REQUIRE(127 == detail::limits_digits<__int128_t>);
#endif
}

struct nondtible {
    ~nondtible() = delete;
};

struct nondfctible {
    nondfctible() = delete;
};

TEST_CASE("semi_regular")
{
    REQUIRE(!is_semi_regular_v<void>);
    REQUIRE(!is_semi_regular_v<void(int)>);
    REQUIRE(is_semi_regular_v<int>);
    REQUIRE(is_semi_regular_v<int *>);
    REQUIRE(!is_semi_regular_v<int &>);
    REQUIRE(!is_semi_regular_v<const int>);
    REQUIRE(!is_semi_regular_v<const int &>);
    REQUIRE(!is_semi_regular_v<int &&>);
    REQUIRE(!is_semi_regular_v<nondtible>);
    REQUIRE(!is_semi_regular_v<nondfctible>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!SemiRegular<void>);
    REQUIRE(!SemiRegular<void(int)>);
    REQUIRE(SemiRegular<int>);
    REQUIRE(SemiRegular<int *>);
    REQUIRE(!SemiRegular<int &>);
    REQUIRE(!SemiRegular<const int>);
    REQUIRE(!SemiRegular<const int &>);
    REQUIRE(!SemiRegular<int &&>);
    REQUIRE(!SemiRegular<nondtible>);
    REQUIRE(!SemiRegular<nondfctible>);
#endif
}

struct non_si_00 {
};

struct non_si_01 {
};

// Wrong ret type.
int operator<<(std::ostream &, const non_si_01 &);

struct part_si {
};

std::ostream &operator<<(std::ostream &, part_si &&);

struct yes_si {
};

std::ostream &operator<<(std::ostream &, const yes_si &);

TEST_CASE("stream_insertable")
{
    REQUIRE(!is_stream_insertable_v<void>);

    REQUIRE(is_stream_insertable_v<int>);
    REQUIRE(is_stream_insertable_v<int &>);
    REQUIRE(is_stream_insertable_v<const int &>);
    REQUIRE(is_stream_insertable_v<int &&>);

    REQUIRE(is_stream_insertable_v<std::string>);
    REQUIRE(is_stream_insertable_v<std::string &>);
    REQUIRE(is_stream_insertable_v<std::string &>);
    REQUIRE(is_stream_insertable_v<std::string &&>);

    REQUIRE(!is_stream_insertable_v<const non_si_00 &>);
    REQUIRE(!is_stream_insertable_v<const non_si_01 &>);

    REQUIRE(!is_stream_insertable_v<const part_si &>);
    REQUIRE(is_stream_insertable_v<part_si &&>);

    REQUIRE(is_stream_insertable_v<yes_si>);
    REQUIRE(is_stream_insertable_v<yes_si &>);
    REQUIRE(is_stream_insertable_v<yes_si &>);
    REQUIRE(is_stream_insertable_v<yes_si &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!StreamInsertable<void>);

    REQUIRE(StreamInsertable<int>);
    REQUIRE(StreamInsertable<int &>);
    REQUIRE(StreamInsertable<const int &>);
    REQUIRE(StreamInsertable<int &&>);

    REQUIRE(StreamInsertable<std::string>);
    REQUIRE(StreamInsertable<std::string &>);
    REQUIRE(StreamInsertable<std::string &>);
    REQUIRE(StreamInsertable<std::string &&>);

    REQUIRE(!StreamInsertable<const non_si_00 &>);
    REQUIRE(!StreamInsertable<const non_si_01 &>);

    REQUIRE(!StreamInsertable<const part_si &>);
    REQUIRE(StreamInsertable<part_si &&>);

    REQUIRE(StreamInsertable<yes_si>);
    REQUIRE(StreamInsertable<yes_si &>);
    REQUIRE(StreamInsertable<yes_si &>);
    REQUIRE(StreamInsertable<yes_si &&>);
#endif
}

TEST_CASE("in_place_addable")
{
    REQUIRE(!is_in_place_addable_v<void, void>);
    REQUIRE(!is_in_place_addable_v<void, int>);
    REQUIRE(!is_in_place_addable_v<int, void>);

    REQUIRE(is_in_place_addable_v<int &, int>);
    REQUIRE(is_in_place_addable_v<int &, int &>);
    REQUIRE(is_in_place_addable_v<int &, const int &>);
    REQUIRE(is_in_place_addable_v<int &, int &&>);

    REQUIRE(!is_in_place_addable_v<int &&, int>);
    REQUIRE(!is_in_place_addable_v<int &&, int &>);
    REQUIRE(!is_in_place_addable_v<int &&, const int &>);
    REQUIRE(!is_in_place_addable_v<int &&, int &&>);

    REQUIRE(!is_in_place_addable_v<const int &, int>);
    REQUIRE(!is_in_place_addable_v<const int &, int &>);
    REQUIRE(!is_in_place_addable_v<const int &, const int &>);
    REQUIRE(!is_in_place_addable_v<const int &, int &&>);

    REQUIRE(!is_in_place_addable_v<int, int>);
    REQUIRE(!is_in_place_addable_v<int, int &>);
    REQUIRE(!is_in_place_addable_v<int, const int &>);
    REQUIRE(!is_in_place_addable_v<int, int &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!InPlaceAddable<void, void>);
    REQUIRE(!InPlaceAddable<void, int>);
    REQUIRE(!InPlaceAddable<int, void>);

    REQUIRE(InPlaceAddable<int &, int>);
    REQUIRE(InPlaceAddable<int &, int &>);
    REQUIRE(InPlaceAddable<int &, const int &>);
    REQUIRE(InPlaceAddable<int &, int &&>);

    REQUIRE(!InPlaceAddable<int &&, int>);
    REQUIRE(!InPlaceAddable<int &&, int &>);
    REQUIRE(!InPlaceAddable<int &&, const int &>);
    REQUIRE(!InPlaceAddable<int &&, int &&>);

    REQUIRE(!InPlaceAddable<const int &, int>);
    REQUIRE(!InPlaceAddable<const int &, int &>);
    REQUIRE(!InPlaceAddable<const int &, const int &>);
    REQUIRE(!InPlaceAddable<const int &, int &&>);

    REQUIRE(!InPlaceAddable<int, int>);
    REQUIRE(!InPlaceAddable<int, int &>);
    REQUIRE(!InPlaceAddable<int, const int &>);
    REQUIRE(!InPlaceAddable<int, int &&>);
#endif
}

struct nonsubtractable_0 {
};

struct subtractable_0 {
    friend subtractable_0 operator-(subtractable_0, subtractable_0);
};

struct subtractable_1 {
    friend subtractable_1 operator-(subtractable_1, subtractable_0);
    friend subtractable_1 operator-(subtractable_0, subtractable_1);
};

struct nonsubtractable_1 {
    friend nonsubtractable_1 operator-(nonsubtractable_1, subtractable_0);
};

struct nonsubtractable_2 {
    friend nonsubtractable_2 operator-(nonsubtractable_2, subtractable_0);
    friend nonsubtractable_1 operator-(subtractable_0, nonsubtractable_2);
};

TEST_CASE("is_subtractable")
{
    REQUIRE(!is_subtractable_v<void>);
    REQUIRE(!is_subtractable_v<void, void>);
    REQUIRE(!is_subtractable_v<void, int>);
    REQUIRE(!is_subtractable_v<int, void>);
    REQUIRE(is_subtractable_v<int>);
    REQUIRE(is_subtractable_v<int, int>);
    REQUIRE(is_subtractable_v<const int, int &>);
    REQUIRE(is_subtractable_v<int &&, volatile int &>);
    REQUIRE(!is_subtractable_v<std::string, char *>);
    REQUIRE(!is_subtractable_v<std::string, int>);
    REQUIRE(!is_subtractable_v<nonsubtractable_0>);
    REQUIRE(is_subtractable_v<subtractable_0>);
    REQUIRE(is_subtractable_v<subtractable_1, subtractable_0>);
    REQUIRE(!is_subtractable_v<nonsubtractable_1, subtractable_0>);
    REQUIRE(!is_subtractable_v<nonsubtractable_2, subtractable_0>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Subtractable<void>);
    REQUIRE(!Subtractable<void, void>);
    REQUIRE(!Subtractable<void, int>);
    REQUIRE(!Subtractable<int, void>);
    REQUIRE(Subtractable<int>);
    REQUIRE(Subtractable<int, int>);
    REQUIRE(Subtractable<const int, int &>);
    REQUIRE(Subtractable<int &&, volatile int &>);
    REQUIRE(!Subtractable<std::string, char *>);
    REQUIRE(!Subtractable<std::string, int>);
    REQUIRE(!Subtractable<nonsubtractable_0>);
    REQUIRE(Subtractable<subtractable_0>);
    REQUIRE(Subtractable<subtractable_1, subtractable_0>);
    REQUIRE(!Subtractable<nonsubtractable_1, subtractable_0>);
    REQUIRE(!Subtractable<nonsubtractable_2, subtractable_0>);
#endif
}

TEST_CASE("in_place_subtractable")
{
    REQUIRE(!is_in_place_subtractable_v<void, void>);
    REQUIRE(!is_in_place_subtractable_v<void, int>);
    REQUIRE(!is_in_place_subtractable_v<int, void>);

    REQUIRE(is_in_place_subtractable_v<int &, int>);
    REQUIRE(is_in_place_subtractable_v<int &, int &>);
    REQUIRE(is_in_place_subtractable_v<int &, const int &>);
    REQUIRE(is_in_place_subtractable_v<int &, int &&>);

    REQUIRE(!is_in_place_subtractable_v<int &&, int>);
    REQUIRE(!is_in_place_subtractable_v<int &&, int &>);
    REQUIRE(!is_in_place_subtractable_v<int &&, const int &>);
    REQUIRE(!is_in_place_subtractable_v<int &&, int &&>);

    REQUIRE(!is_in_place_subtractable_v<const int &, int>);
    REQUIRE(!is_in_place_subtractable_v<const int &, int &>);
    REQUIRE(!is_in_place_subtractable_v<const int &, const int &>);
    REQUIRE(!is_in_place_subtractable_v<const int &, int &&>);

    REQUIRE(!is_in_place_subtractable_v<int, int>);
    REQUIRE(!is_in_place_subtractable_v<int, int &>);
    REQUIRE(!is_in_place_subtractable_v<int, const int &>);
    REQUIRE(!is_in_place_subtractable_v<int, int &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!InPlaceSubtractable<void, void>);
    REQUIRE(!InPlaceSubtractable<void, int>);
    REQUIRE(!InPlaceSubtractable<int, void>);

    REQUIRE(InPlaceSubtractable<int &, int>);
    REQUIRE(InPlaceSubtractable<int &, int &>);
    REQUIRE(InPlaceSubtractable<int &, const int &>);
    REQUIRE(InPlaceSubtractable<int &, int &&>);

    REQUIRE(!InPlaceSubtractable<int &&, int>);
    REQUIRE(!InPlaceSubtractable<int &&, int &>);
    REQUIRE(!InPlaceSubtractable<int &&, const int &>);
    REQUIRE(!InPlaceSubtractable<int &&, int &&>);

    REQUIRE(!InPlaceSubtractable<const int &, int>);
    REQUIRE(!InPlaceSubtractable<const int &, int &>);
    REQUIRE(!InPlaceSubtractable<const int &, const int &>);
    REQUIRE(!InPlaceSubtractable<const int &, int &&>);

    REQUIRE(!InPlaceSubtractable<int, int>);
    REQUIRE(!InPlaceSubtractable<int, int &>);
    REQUIRE(!InPlaceSubtractable<int, const int &>);
    REQUIRE(!InPlaceSubtractable<int, int &&>);
#endif
}

struct defstr00 {
    template <typename... Args>
    defstr00(Args &&...);
};

struct nondestr00 {
    nondestr00() = default;
    ~nondestr00() = delete;
};

TEST_CASE("constructible")
{
#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Constructible<void>);

    REQUIRE(Constructible<int>);
    REQUIRE(!Constructible<int, void>);
    REQUIRE(!Constructible<int &>);
    REQUIRE(!Constructible<int &&>);
    REQUIRE(!Constructible<const int &>);

    REQUIRE(Constructible<defstr00>);
    REQUIRE(Constructible<defstr00, int, int>);
    REQUIRE(Constructible<defstr00, int &, const int &>);

    REQUIRE(!Constructible<nondestr00>);
#endif
}

TEST_CASE("mutable_rvalue_reference")
{
    REQUIRE(!is_mutable_rvalue_reference_v<void>);

    REQUIRE(!is_mutable_rvalue_reference_v<int>);
    REQUIRE(!is_mutable_rvalue_reference_v<int &>);
    REQUIRE(!is_mutable_rvalue_reference_v<const int &>);
    REQUIRE(!is_mutable_rvalue_reference_v<const int &&>);
    REQUIRE(is_mutable_rvalue_reference_v<int &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!MutableRvalueReference<void>);

    REQUIRE(!MutableRvalueReference<int>);
    REQUIRE(!MutableRvalueReference<int &>);
    REQUIRE(!MutableRvalueReference<const int &>);
    REQUIRE(!MutableRvalueReference<const int &&>);
    REQUIRE(MutableRvalueReference<int &&>);
#endif
}

struct nonmultipliable_0 {
};

struct multipliable_0 {
    friend multipliable_0 operator*(multipliable_0, multipliable_0);
};

struct multipliable_1 {
    friend multipliable_1 operator*(multipliable_1, multipliable_0);
    friend multipliable_1 operator*(multipliable_0, multipliable_1);
};

struct nonmultipliable_1 {
    friend nonmultipliable_1 operator*(nonmultipliable_1, multipliable_0);
};

struct nonmultipliable_2 {
    friend nonmultipliable_2 operator*(nonmultipliable_2, multipliable_0);
    friend nonmultipliable_1 operator*(multipliable_0, nonmultipliable_2);
};

TEST_CASE("is_multipliable")
{
    REQUIRE(!is_multipliable_v<void>);
    REQUIRE(!is_multipliable_v<void, void>);
    REQUIRE(!is_multipliable_v<void, int>);
    REQUIRE(!is_multipliable_v<int, void>);
    REQUIRE(is_multipliable_v<int>);
    REQUIRE(is_multipliable_v<int, int>);
    REQUIRE(is_multipliable_v<const int, int &>);
    REQUIRE(is_multipliable_v<int &&, volatile int &>);
    REQUIRE(!is_multipliable_v<std::string, char *>);
    REQUIRE(!is_multipliable_v<std::string, int>);
    REQUIRE(!is_multipliable_v<nonmultipliable_0>);
    REQUIRE(is_multipliable_v<multipliable_0>);
    REQUIRE(is_multipliable_v<multipliable_1, multipliable_0>);
    REQUIRE(!is_multipliable_v<nonmultipliable_1, multipliable_0>);
    REQUIRE(!is_multipliable_v<nonmultipliable_2, multipliable_0>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Multipliable<void>);
    REQUIRE(!Multipliable<void, void>);
    REQUIRE(!Multipliable<void, int>);
    REQUIRE(!Multipliable<int, void>);
    REQUIRE(Multipliable<int>);
    REQUIRE(Multipliable<int, int>);
    REQUIRE(Multipliable<const int, int &>);
    REQUIRE(Multipliable<int &&, volatile int &>);
    REQUIRE(!Multipliable<std::string, char *>);
    REQUIRE(!Multipliable<std::string, int>);
    REQUIRE(!Multipliable<nonmultipliable_0>);
    REQUIRE(Multipliable<multipliable_0>);
    REQUIRE(Multipliable<multipliable_1, multipliable_0>);
    REQUIRE(!Multipliable<nonmultipliable_1, multipliable_0>);
    REQUIRE(!Multipliable<nonmultipliable_2, multipliable_0>);
#endif
}

TEST_CASE("in_place_multipliable")
{
    REQUIRE(!is_in_place_multipliable_v<void, void>);
    REQUIRE(!is_in_place_multipliable_v<void, int>);
    REQUIRE(!is_in_place_multipliable_v<int, void>);

    REQUIRE(is_in_place_multipliable_v<int &, int>);
    REQUIRE(is_in_place_multipliable_v<int &, int &>);
    REQUIRE(is_in_place_multipliable_v<int &, const int &>);
    REQUIRE(is_in_place_multipliable_v<int &, int &&>);

    REQUIRE(!is_in_place_multipliable_v<int &&, int>);
    REQUIRE(!is_in_place_multipliable_v<int &&, int &>);
    REQUIRE(!is_in_place_multipliable_v<int &&, const int &>);
    REQUIRE(!is_in_place_multipliable_v<int &&, int &&>);

    REQUIRE(!is_in_place_multipliable_v<const int &, int>);
    REQUIRE(!is_in_place_multipliable_v<const int &, int &>);
    REQUIRE(!is_in_place_multipliable_v<const int &, const int &>);
    REQUIRE(!is_in_place_multipliable_v<const int &, int &&>);

    REQUIRE(!is_in_place_multipliable_v<int, int>);
    REQUIRE(!is_in_place_multipliable_v<int, int &>);
    REQUIRE(!is_in_place_multipliable_v<int, const int &>);
    REQUIRE(!is_in_place_multipliable_v<int, int &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!InPlaceMultipliable<void, void>);
    REQUIRE(!InPlaceMultipliable<void, int>);
    REQUIRE(!InPlaceMultipliable<int, void>);

    REQUIRE(InPlaceMultipliable<int &, int>);
    REQUIRE(InPlaceMultipliable<int &, int &>);
    REQUIRE(InPlaceMultipliable<int &, const int &>);
    REQUIRE(InPlaceMultipliable<int &, int &&>);

    REQUIRE(!InPlaceMultipliable<int &&, int>);
    REQUIRE(!InPlaceMultipliable<int &&, int &>);
    REQUIRE(!InPlaceMultipliable<int &&, const int &>);
    REQUIRE(!InPlaceMultipliable<int &&, int &&>);

    REQUIRE(!InPlaceMultipliable<const int &, int>);
    REQUIRE(!InPlaceMultipliable<const int &, int &>);
    REQUIRE(!InPlaceMultipliable<const int &, const int &>);
    REQUIRE(!InPlaceMultipliable<const int &, int &&>);

    REQUIRE(!InPlaceMultipliable<int, int>);
    REQUIRE(!InPlaceMultipliable<int, int &>);
    REQUIRE(!InPlaceMultipliable<int, const int &>);
    REQUIRE(!InPlaceMultipliable<int, int &&>);
#endif
}

struct nondivisible_0 {
};

struct divisible_0 {
    friend divisible_0 operator/(divisible_0, divisible_0);
};

struct divisible_1 {
    friend divisible_1 operator/(divisible_1, divisible_0);
    friend divisible_1 operator/(divisible_0, divisible_1);
};

struct nondivisible_1 {
    friend nondivisible_1 operator/(nondivisible_1, divisible_0);
};

struct nondivisible_2 {
    friend nondivisible_2 operator/(nondivisible_2, divisible_0);
    friend nondivisible_1 operator/(divisible_0, nondivisible_2);
};

TEST_CASE("is_divisible")
{
    REQUIRE(!is_divisible_v<void>);
    REQUIRE(!is_divisible_v<void, void>);
    REQUIRE(!is_divisible_v<void, int>);
    REQUIRE(!is_divisible_v<int, void>);
    REQUIRE(is_divisible_v<int>);
    REQUIRE(is_divisible_v<int, int>);
    REQUIRE(is_divisible_v<const int, int &>);
    REQUIRE(is_divisible_v<int &&, volatile int &>);
    REQUIRE(!is_divisible_v<std::string, char *>);
    REQUIRE(!is_divisible_v<std::string, int>);
    REQUIRE(!is_divisible_v<nondivisible_0>);
    REQUIRE(is_divisible_v<divisible_0>);
    REQUIRE(is_divisible_v<divisible_1, divisible_0>);
    REQUIRE(!is_divisible_v<nondivisible_1, divisible_0>);
    REQUIRE(!is_divisible_v<nondivisible_2, divisible_0>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Divisible<void>);
    REQUIRE(!Divisible<void, void>);
    REQUIRE(!Divisible<void, int>);
    REQUIRE(!Divisible<int, void>);
    REQUIRE(Divisible<int>);
    REQUIRE(Divisible<int, int>);
    REQUIRE(Divisible<const int, int &>);
    REQUIRE(Divisible<int &&, volatile int &>);
    REQUIRE(!Divisible<std::string, char *>);
    REQUIRE(!Divisible<std::string, int>);
    REQUIRE(!Divisible<nondivisible_0>);
    REQUIRE(Divisible<divisible_0>);
    REQUIRE(Divisible<divisible_1, divisible_0>);
    REQUIRE(!Divisible<nondivisible_1, divisible_0>);
    REQUIRE(!Divisible<nondivisible_2, divisible_0>);
#endif
}

TEST_CASE("in_place_divisible")
{
    REQUIRE(!is_in_place_divisible_v<void, void>);
    REQUIRE(!is_in_place_divisible_v<void, int>);
    REQUIRE(!is_in_place_divisible_v<int, void>);

    REQUIRE(is_in_place_divisible_v<int &, int>);
    REQUIRE(is_in_place_divisible_v<int &, int &>);
    REQUIRE(is_in_place_divisible_v<int &, const int &>);
    REQUIRE(is_in_place_divisible_v<int &, int &&>);

    REQUIRE(!is_in_place_divisible_v<int &&, int>);
    REQUIRE(!is_in_place_divisible_v<int &&, int &>);
    REQUIRE(!is_in_place_divisible_v<int &&, const int &>);
    REQUIRE(!is_in_place_divisible_v<int &&, int &&>);

    REQUIRE(!is_in_place_divisible_v<const int &, int>);
    REQUIRE(!is_in_place_divisible_v<const int &, int &>);
    REQUIRE(!is_in_place_divisible_v<const int &, const int &>);
    REQUIRE(!is_in_place_divisible_v<const int &, int &&>);

    REQUIRE(!is_in_place_divisible_v<int, int>);
    REQUIRE(!is_in_place_divisible_v<int, int &>);
    REQUIRE(!is_in_place_divisible_v<int, const int &>);
    REQUIRE(!is_in_place_divisible_v<int, int &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!InPlaceDivisible<void, void>);
    REQUIRE(!InPlaceDivisible<void, int>);
    REQUIRE(!InPlaceDivisible<int, void>);

    REQUIRE(InPlaceDivisible<int &, int>);
    REQUIRE(InPlaceDivisible<int &, int &>);
    REQUIRE(InPlaceDivisible<int &, const int &>);
    REQUIRE(InPlaceDivisible<int &, int &&>);

    REQUIRE(!InPlaceDivisible<int &&, int>);
    REQUIRE(!InPlaceDivisible<int &&, int &>);
    REQUIRE(!InPlaceDivisible<int &&, const int &>);
    REQUIRE(!InPlaceDivisible<int &&, int &&>);

    REQUIRE(!InPlaceDivisible<const int &, int>);
    REQUIRE(!InPlaceDivisible<const int &, int &>);
    REQUIRE(!InPlaceDivisible<const int &, const int &>);
    REQUIRE(!InPlaceDivisible<const int &, int &&>);

    REQUIRE(!InPlaceDivisible<int, int>);
    REQUIRE(!InPlaceDivisible<int, int &>);
    REQUIRE(!InPlaceDivisible<int, const int &>);
    REQUIRE(!InPlaceDivisible<int, int &&>);
#endif
}
