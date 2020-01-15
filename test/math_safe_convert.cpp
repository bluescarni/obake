// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <limits>
#include <string>
#include <type_traits>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/math/safe_convert.hpp>
#include <obake/type_traits.hpp>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include "catch.hpp"

TEST_CASE("safe_convert_integrals")
{
    int n;
    unsigned u;

    // Same signedness, same types. Conversion will always succeed.
    REQUIRE(obake::safe_convert(n, 45));
    REQUIRE(n == 45);
    REQUIRE(obake::safe_convert(n, -45));
    REQUIRE(n == -45);
    REQUIRE(obake::safe_convert(u, 41u));
    REQUIRE(u == 41u);

    // Same width, but signed vs unsigned.
    REQUIRE(obake::safe_convert(n, 55u));
    REQUIRE(n == 55);
    REQUIRE(obake::safe_convert(u, 54));
    REQUIRE(u == 54u);
    REQUIRE(!obake::safe_convert(u, -54));
    REQUIRE(u == 54u);
    REQUIRE(!obake::safe_convert(n, std::numeric_limits<unsigned>::max()));
    REQUIRE(n == 55);

    // Different types with different ranges.
    if constexpr (std::numeric_limits<int>::max() < std::numeric_limits<long>::max()
                  && std::numeric_limits<unsigned>::max() < std::numeric_limits<unsigned long>::max()) {
        long l;
        unsigned long ul;

        // Int to long, success.
        REQUIRE(obake::safe_convert(l, 42));
        REQUIRE(l == 42);
        // Long to int, success.
        REQUIRE(obake::safe_convert(n, 40l));
        REQUIRE(n == 40);
        // Long to int, failure.
        REQUIRE(!obake::safe_convert(n, std::numeric_limits<long>::max()));
        REQUIRE(n == 40);

        // Unsigned to ulong, success.
        REQUIRE(obake::safe_convert(ul, 38u));
        REQUIRE(ul == 38u);
        // Ulong to unsigned, success.
        REQUIRE(obake::safe_convert(u, 36ul));
        REQUIRE(u == 36u);
        // Ulong to unsigned, failure.
        REQUIRE(!obake::safe_convert(u, std::numeric_limits<unsigned long>::max()));
        REQUIRE(u == 36u);
        // Ulong to long, failure.
        REQUIRE(!obake::safe_convert(l, std::numeric_limits<unsigned long>::max()));
        REQUIRE(l == 42);
    }

    // Couple of tests with bool.
    bool b;
    REQUIRE(obake::safe_convert(b, 0));
    REQUIRE(!b);
    REQUIRE(obake::safe_convert(b, 1));
    REQUIRE(b);
    REQUIRE(!obake::safe_convert(b, -1));
    REQUIRE(b);
    REQUIRE(!obake::safe_convert(b, 2));
    REQUIRE(b);
    REQUIRE(obake::safe_convert(b, 0));
    REQUIRE(!b);
    REQUIRE(obake::safe_convert(n, true));
    REQUIRE(n == 1);
    REQUIRE(obake::safe_convert(n, false));
    REQUIRE(n == 0);

#if defined(OBAKE_HAVE_GCC_INT128)

    __int128_t i128;
    __uint128_t ui128;
    REQUIRE(obake::safe_convert(i128, 45));
    REQUIRE(i128 == 45);
    REQUIRE(obake::safe_convert(i128, -45));
    REQUIRE(i128 == -45);
    REQUIRE(obake::safe_convert(ui128, 45));
    REQUIRE(ui128 == 45u);
    REQUIRE(!obake::safe_convert(ui128, -44));
    REQUIRE(ui128 == 45u);
    REQUIRE(obake::safe_convert(n, __int128_t{31}));
    REQUIRE(n == 31);
    REQUIRE(obake::safe_convert(n, __uint128_t{31}));
    REQUIRE(n == 31);
    REQUIRE(obake::safe_convert(n, __int128_t{-31}));
    REQUIRE(n == -31);
    REQUIRE(obake::safe_convert(u, __uint128_t{30}));
    REQUIRE(u == 30u);
    REQUIRE(obake::safe_convert(u, __int128_t{30}));
    REQUIRE(u == 30u);
    REQUIRE(!obake::safe_convert(u, __int128_t{-30}));
    REQUIRE(u == 30u);

    if constexpr ((obake::detail::limits_max<__int128_t>) > std::numeric_limits<int>::max()
                  && (obake::detail::limits_max<__uint128_t>) > std::numeric_limits<unsigned>::max()) {
        REQUIRE(!obake::safe_convert(n, obake::detail::limits_max<__int128_t>));
        REQUIRE(!obake::safe_convert(u, obake::detail::limits_max<__uint128_t>));
    }

#endif

    REQUIRE(obake::is_safely_convertible_v<int &&, int &>);
    REQUIRE(obake::is_safely_convertible_v<int &, int &>);
    REQUIRE(obake::is_safely_convertible_v<const int &, int &>);
    REQUIRE(!obake::is_safely_convertible_v<double &, int &>);
    REQUIRE(!obake::is_safely_convertible_v<void, void>);
    REQUIRE(!obake::is_safely_convertible_v<int &&, int>);
    REQUIRE(!obake::is_safely_convertible_v<int &&, const int &>);
    REQUIRE(!obake::is_safely_convertible_v<int, void>);
    REQUIRE(!obake::is_safely_convertible_v<void, int>);
    REQUIRE(!obake::is_safely_convertible_v<void, void>);
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::is_safely_convertible_v<int &&, __int128_t &>);
    REQUIRE(obake::is_safely_convertible_v<int &, __uint128_t &>);
    REQUIRE(!obake::is_safely_convertible_v<int &&, const __int128_t &>);
    REQUIRE(!obake::is_safely_convertible_v<int &, __uint128_t &&>);
    REQUIRE(obake::is_safely_convertible_v<__int128_t &&, int &>);
    REQUIRE(obake::is_safely_convertible_v<__uint128_t &, int &>);
    REQUIRE(!obake::is_safely_convertible_v<__int128_t &&, const int &>);
    REQUIRE(!obake::is_safely_convertible_v<__uint128_t &, int &&>);
#endif

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::SafelyConvertible<int &&, int &>);
    REQUIRE(obake::SafelyConvertible<int &, int &>);
    REQUIRE(obake::SafelyConvertible<const int &, int &>);
    REQUIRE(!obake::SafelyConvertible<double &, int &>);
    REQUIRE(!obake::SafelyConvertible<void, void>);
    REQUIRE(!obake::SafelyConvertible<int &&, int>);
    REQUIRE(!obake::SafelyConvertible<int &&, const int &>);
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::SafelyConvertible<int &&, __int128_t &>);
    REQUIRE(obake::SafelyConvertible<int &, __uint128_t &>);
    REQUIRE(!obake::SafelyConvertible<int &&, const __int128_t &>);
    REQUIRE(!obake::SafelyConvertible<int &, __uint128_t &&>);
    REQUIRE(obake::SafelyConvertible<__int128_t &&, int &>);
    REQUIRE(obake::SafelyConvertible<__uint128_t &, int &>);
    REQUIRE(!obake::SafelyConvertible<__int128_t &&, const int &>);
    REQUIRE(!obake::SafelyConvertible<__uint128_t &, int &&>);
#endif
#endif
}

TEST_CASE("safe_convert_mppp_integer")
{
    using int_t = mppp::integer<1>;

    int n;
    REQUIRE(obake::safe_convert(n, int_t{10}));
    REQUIRE(n == 10);
    REQUIRE(obake::safe_convert(n, int_t{-10}));
    REQUIRE(n == -10);
    REQUIRE(!obake::safe_convert(n, int_t{std::numeric_limits<int>::max()} + 1));
    REQUIRE(n == -10);
    REQUIRE(!obake::safe_convert(n, int_t{std::numeric_limits<int>::min()} - 1));
    REQUIRE(n == -10);

    int_t out;
    REQUIRE(obake::safe_convert(out, 10));
    REQUIRE(out == 10);
    REQUIRE(obake::safe_convert(out, -10000l));
    REQUIRE(out == -10000l);

#if defined(OBAKE_HAVE_GCC_INT128)

    __int128_t i128;
    __uint128_t ui128;
    REQUIRE(obake::safe_convert(i128, int_t{45}));
    REQUIRE(i128 == 45);
    REQUIRE(obake::safe_convert(i128, int_t{-45}));
    REQUIRE(i128 == -45);
    REQUIRE(obake::safe_convert(ui128, int_t{45}));
    REQUIRE(ui128 == 45u);
    REQUIRE(!obake::safe_convert(ui128, int_t{-44}));
    REQUIRE(ui128 == 45u);
    REQUIRE(obake::safe_convert(out, __int128_t{33}));
    REQUIRE(out == 33);
    REQUIRE(obake::safe_convert(out, __int128_t{-33}));
    REQUIRE(out == -33);
    REQUIRE(obake::safe_convert(out, __uint128_t{32}));
    REQUIRE(out == 32);
    REQUIRE(!obake::safe_convert(i128, int_t{obake::detail::limits_max<__int128_t>} + 1));
    REQUIRE(!obake::safe_convert(ui128, int_t{obake::detail::limits_max<__uint128_t>} + 1));

#endif

    REQUIRE(obake::is_safely_convertible_v<int &&, int_t &>);
    REQUIRE(obake::is_safely_convertible_v<int &, int_t &>);
    REQUIRE(obake::is_safely_convertible_v<const int &, int_t &>);
    REQUIRE(!obake::is_safely_convertible_v<double &, int_t &>);
    REQUIRE(!obake::is_safely_convertible_v<int &&, int_t>);
    REQUIRE(!obake::is_safely_convertible_v<int &&, const int_t &>);
    REQUIRE(obake::is_safely_convertible_v<int_t &&, int &>);
    REQUIRE(obake::is_safely_convertible_v<int_t &, int &>);
    REQUIRE(obake::is_safely_convertible_v<const int_t &, int &>);
    REQUIRE(!obake::is_safely_convertible_v<int_t &, double &>);
    REQUIRE(!obake::is_safely_convertible_v<int_t &&, int>);
    REQUIRE(!obake::is_safely_convertible_v<int_t &&, const int &>);
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::is_safely_convertible_v<int_t &&, __int128_t &>);
    REQUIRE(obake::is_safely_convertible_v<int_t &, __uint128_t &>);
    REQUIRE(!obake::is_safely_convertible_v<int_t &&, const __int128_t &>);
    REQUIRE(!obake::is_safely_convertible_v<int_t &, __uint128_t &&>);
    REQUIRE(obake::is_safely_convertible_v<__int128_t &&, int_t &>);
    REQUIRE(obake::is_safely_convertible_v<__uint128_t &, int_t &>);
    REQUIRE(!obake::is_safely_convertible_v<__int128_t &&, const int_t &>);
    REQUIRE(!obake::is_safely_convertible_v<__uint128_t &, int_t &&>);
#endif
}

// integer/rational.
TEST_CASE("safe_convert_mppp_integer_rational")
{
    using int_t = mppp::integer<1>;
    using rat_t = mppp::rational<1>;

    int_t n;
    int_t q;

    REQUIRE(obake::safe_convert(n, rat_t{2, 2}));
    REQUIRE(!obake::safe_convert(n, rat_t{2, -3}));

    REQUIRE(obake::safe_convert(q, int_t{3}));
    REQUIRE(obake::safe_convert(n, int_t{-6}));

    REQUIRE(obake::is_safely_convertible_v<rat_t, int_t &>);
    REQUIRE(obake::is_safely_convertible_v<rat_t &&, int_t &>);
    REQUIRE(obake::is_safely_convertible_v<const rat_t &, int_t &>);
    REQUIRE(obake::is_safely_convertible_v<const rat_t, int_t &>);
    REQUIRE(!obake::is_safely_convertible_v<rat_t, const int_t &>);
    REQUIRE(!obake::is_safely_convertible_v<rat_t, int_t>);

    REQUIRE(obake::is_safely_convertible_v<int_t, rat_t &>);
    REQUIRE(obake::is_safely_convertible_v<int_t &&, rat_t &>);
    REQUIRE(obake::is_safely_convertible_v<const int_t &, rat_t &>);
    REQUIRE(obake::is_safely_convertible_v<const int_t, rat_t &>);
    REQUIRE(!obake::is_safely_convertible_v<int_t, const rat_t &>);
    REQUIRE(!obake::is_safely_convertible_v<int_t, rat_t>);
}

// integral/rational.
TEST_CASE("safe_convert_integrals_rational")
{
    using rat_t = mppp::rational<1>;

    int out;
    REQUIRE(obake::safe_convert(out, rat_t{2, 2}));
    REQUIRE(out == 1);
    REQUIRE(obake::safe_convert(out, rat_t{-2, 2}));
    REQUIRE(out == -1);
    REQUIRE(!obake::safe_convert(out, rat_t{2, 3}));
    REQUIRE(out == -1);
    REQUIRE(!obake::safe_convert(out, rat_t{-2, 3}));
    REQUIRE(out == -1);

    unsigned uout;
    REQUIRE(obake::safe_convert(uout, rat_t{2, 2}));
    REQUIRE(uout == 1u);
    REQUIRE(!obake::safe_convert(uout, rat_t{-2, 2}));
    REQUIRE(uout == 1u);
    REQUIRE(!obake::safe_convert(uout, rat_t{2, 3}));
    REQUIRE(uout == 1u);
    REQUIRE(!obake::safe_convert(uout, rat_t{-2, 3}));
    REQUIRE(uout == 1u);

    rat_t r;
    REQUIRE(obake::safe_convert(r, 123));
    REQUIRE(r == 123);
    REQUIRE(obake::safe_convert(r, -123));
    REQUIRE(r == -123);
    REQUIRE(obake::safe_convert(r, 123u));
    REQUIRE(r == 123);
    REQUIRE(obake::safe_convert(r, 123ull));
    REQUIRE(r == 123);

#if defined(OBAKE_HAVE_GCC_INT128)
    __int128_t iout;
    REQUIRE(obake::safe_convert(iout, rat_t{2, 2}));
    REQUIRE(iout == 1);
    REQUIRE(obake::safe_convert(iout, rat_t{-2, 2}));
    REQUIRE(iout == -1);
    REQUIRE(!obake::safe_convert(iout, rat_t{2, 3}));
    REQUIRE(iout == -1);
    REQUIRE(!obake::safe_convert(iout, rat_t{-2, 3}));
    REQUIRE(iout == -1);

    __uint128_t uiout;
    REQUIRE(obake::safe_convert(uiout, rat_t{2, 2}));
    REQUIRE(uiout == 1);
    REQUIRE(!obake::safe_convert(uiout, rat_t{-2, 2}));
    REQUIRE(uiout == 1);
    REQUIRE(!obake::safe_convert(uiout, rat_t{2, 3}));
    REQUIRE(uiout == 1);
    REQUIRE(!obake::safe_convert(uiout, rat_t{-2, 3}));
    REQUIRE(uiout == 1);
#endif

    REQUIRE(obake::is_safely_convertible_v<rat_t, int &>);
    REQUIRE(obake::is_safely_convertible_v<rat_t &, int &>);
    REQUIRE(obake::is_safely_convertible_v<const rat_t &, int &>);
    REQUIRE(!obake::is_safely_convertible_v<const rat_t &, const int &>);
    REQUIRE(!obake::is_safely_convertible_v<const rat_t &, int &&>);
    REQUIRE(obake::is_safely_convertible_v<int, rat_t &>);
    REQUIRE(obake::is_safely_convertible_v<int &, rat_t &>);
    REQUIRE(obake::is_safely_convertible_v<const int &, rat_t &>);
    REQUIRE(!obake::is_safely_convertible_v<const int &, const rat_t &>);
    REQUIRE(!obake::is_safely_convertible_v<const int &, rat_t>);

    REQUIRE(obake::is_safely_convertible_v<rat_t, unsigned &>);
    REQUIRE(obake::is_safely_convertible_v<rat_t &, unsigned &>);
    REQUIRE(obake::is_safely_convertible_v<const rat_t &, unsigned &>);
    REQUIRE(!obake::is_safely_convertible_v<const rat_t &, const unsigned &>);
    REQUIRE(!obake::is_safely_convertible_v<const rat_t &, unsigned &&>);
    REQUIRE(obake::is_safely_convertible_v<unsigned, rat_t &>);
    REQUIRE(obake::is_safely_convertible_v<unsigned &, rat_t &>);
    REQUIRE(obake::is_safely_convertible_v<const unsigned &, rat_t &>);
    REQUIRE(!obake::is_safely_convertible_v<const unsigned &, const rat_t &>);
    REQUIRE(!obake::is_safely_convertible_v<const unsigned &, rat_t>);
}

// Test the customisation machinery.

// A new type.
struct foo0 {
};

// Customise obake::safe_convert() for foo0.
namespace obake::customisation
{

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, foo0> &&SameCvr<U, foo0> inline constexpr auto safe_convert<T, U>
#else
inline constexpr auto safe_convert<T, U, std::enable_if_t<is_same_cvr_v<T, foo0> && is_same_cvr_v<U, foo0>>>
#endif
    = [](auto &&, auto &&) constexpr noexcept
{
    return true;
};

} // namespace obake::customisation

TEST_CASE("safe_convert_custom")
{
    REQUIRE(!obake::is_safely_convertible_v<foo0, int>);
    REQUIRE(!obake::is_safely_convertible_v<int, foo0>);
    REQUIRE(obake::is_safely_convertible_v<foo0, foo0>);

    REQUIRE(obake::safe_convert(foo0{}, foo0{}));
}

struct foo1 {
};

// A non-assignable struct.
struct foo2 {
    foo2 operator=(const foo2 &) = delete;
    foo2 operator=(foo2 &&) = delete;
};

// Default implementation for conversion to the same type.
TEST_CASE("safe_convert_same")
{
    REQUIRE(obake::is_safely_convertible_v<std::string, std::string &>);
    REQUIRE(obake::is_safely_convertible_v<const std::string &, std::string &>);
    REQUIRE(obake::is_safely_convertible_v<std::string &&, std::string &>);
    REQUIRE(!obake::is_safely_convertible_v<std::string &&, const std::string &>);
    REQUIRE(obake::is_safely_convertible_v<std::string &&, std::string &&>);

    REQUIRE(obake::is_safely_convertible_v<foo1, foo1 &>);
    REQUIRE(obake::is_safely_convertible_v<const foo1 &, foo1 &>);
    REQUIRE(obake::is_safely_convertible_v<foo1 &&, foo1 &>);
    REQUIRE(!obake::is_safely_convertible_v<foo1 &&, const foo1 &>);
    REQUIRE(obake::is_safely_convertible_v<foo1 &&, foo1 &&>);

    REQUIRE(!obake::is_safely_convertible_v<foo2, foo2 &>);
    REQUIRE(!obake::is_safely_convertible_v<const foo2 &, foo2 &>);
    REQUIRE(!obake::is_safely_convertible_v<foo2 &&, foo2 &>);
    REQUIRE(!obake::is_safely_convertible_v<foo2 &&, const foo2 &>);
    REQUIRE(!obake::is_safely_convertible_v<foo2 &&, foo2 &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::SafelyConvertible<std::string, std::string &>);
    REQUIRE(obake::SafelyConvertible<const std::string &, std::string &>);
    REQUIRE(obake::SafelyConvertible<std::string &&, std::string &>);
    REQUIRE(!obake::SafelyConvertible<std::string &&, const std::string &>);
    REQUIRE(obake::SafelyConvertible<std::string &&, std::string &&>);

    REQUIRE(obake::SafelyConvertible<foo1, foo1 &>);
    REQUIRE(obake::SafelyConvertible<const foo1 &, foo1 &>);
    REQUIRE(obake::SafelyConvertible<foo1 &&, foo1 &>);
    REQUIRE(!obake::SafelyConvertible<foo1 &&, const foo1 &>);
    REQUIRE(obake::SafelyConvertible<foo1 &&, foo1 &&>);

    REQUIRE(!obake::SafelyConvertible<foo2, foo2 &>);
    REQUIRE(!obake::SafelyConvertible<const foo2 &, foo2 &>);
    REQUIRE(!obake::SafelyConvertible<foo2 &&, foo2 &>);
    REQUIRE(!obake::SafelyConvertible<foo2 &&, const foo2 &>);
    REQUIRE(!obake::SafelyConvertible<foo2 &&, foo2 &&>);
#endif

    std::string s = "hello";
    REQUIRE(obake::safe_convert(s, std::string{"world"}));
    REQUIRE(s == "world");
}
