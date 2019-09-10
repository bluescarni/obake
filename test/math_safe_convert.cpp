// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <limits>
#include <string>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/math/safe_convert.hpp>
#include <piranha/type_traits.hpp>

#include <mp++/integer.hpp>

#include "catch.hpp"

TEST_CASE("safe_convert_integrals")
{
    int n;
    unsigned u;

    // Same signedness, same types. Conversion will always succeed.
    REQUIRE(piranha::safe_convert(n, 45));
    REQUIRE(n == 45);
    REQUIRE(piranha::safe_convert(n, -45));
    REQUIRE(n == -45);
    REQUIRE(piranha::safe_convert(u, 41u));
    REQUIRE(u == 41u);

    // Same width, but signed vs unsigned.
    REQUIRE(piranha::safe_convert(n, 55u));
    REQUIRE(n == 55);
    REQUIRE(piranha::safe_convert(u, 54));
    REQUIRE(u == 54u);
    REQUIRE(!piranha::safe_convert(u, -54));
    REQUIRE(u == 54u);
    REQUIRE(!piranha::safe_convert(n, std::numeric_limits<unsigned>::max()));
    REQUIRE(n == 55);

    // Different types with different ranges.
    if constexpr (std::numeric_limits<int>::max() < std::numeric_limits<long>::max()
                  && std::numeric_limits<unsigned>::max() < std::numeric_limits<unsigned long>::max()) {
        long l;
        unsigned long ul;

        // Int to long, success.
        REQUIRE(piranha::safe_convert(l, 42));
        REQUIRE(l == 42);
        // Long to int, success.
        REQUIRE(piranha::safe_convert(n, 40l));
        REQUIRE(n == 40);
        // Long to int, failure.
        REQUIRE(!piranha::safe_convert(n, std::numeric_limits<long>::max()));
        REQUIRE(n == 40);

        // Unsigned to ulong, success.
        REQUIRE(piranha::safe_convert(ul, 38u));
        REQUIRE(ul == 38u);
        // Ulong to unsigned, success.
        REQUIRE(piranha::safe_convert(u, 36ul));
        REQUIRE(u == 36u);
        // Ulong to unsigned, failure.
        REQUIRE(!piranha::safe_convert(u, std::numeric_limits<unsigned long>::max()));
        REQUIRE(u == 36u);
        // Ulong to long, failure.
        REQUIRE(!piranha::safe_convert(l, std::numeric_limits<unsigned long>::max()));
        REQUIRE(l == 42);
    }

    // Couple of tests with bool.
    bool b;
    REQUIRE(piranha::safe_convert(b, 0));
    REQUIRE(!b);
    REQUIRE(piranha::safe_convert(b, 1));
    REQUIRE(b);
    REQUIRE(!piranha::safe_convert(b, -1));
    REQUIRE(b);
    REQUIRE(!piranha::safe_convert(b, 2));
    REQUIRE(b);
    REQUIRE(piranha::safe_convert(b, 0));
    REQUIRE(!b);
    REQUIRE(piranha::safe_convert(n, true));
    REQUIRE(n == 1);
    REQUIRE(piranha::safe_convert(n, false));
    REQUIRE(n == 0);

#if defined(PIRANHA_HAVE_GCC_INT128)

    __int128_t i128;
    __uint128_t ui128;
    REQUIRE(piranha::safe_convert(i128, 45));
    REQUIRE(i128 == 45);
    REQUIRE(piranha::safe_convert(i128, -45));
    REQUIRE(i128 == -45);
    REQUIRE(piranha::safe_convert(ui128, 45));
    REQUIRE(ui128 == 45u);
    REQUIRE(!piranha::safe_convert(ui128, -44));
    REQUIRE(ui128 == 45u);
    REQUIRE(piranha::safe_convert(n, __int128_t{31}));
    REQUIRE(n == 31);
    REQUIRE(piranha::safe_convert(n, __uint128_t{31}));
    REQUIRE(n == 31);
    REQUIRE(piranha::safe_convert(n, __int128_t{-31}));
    REQUIRE(n == -31);
    REQUIRE(piranha::safe_convert(u, __uint128_t{30}));
    REQUIRE(u == 30u);
    REQUIRE(piranha::safe_convert(u, __int128_t{30}));
    REQUIRE(u == 30u);
    REQUIRE(!piranha::safe_convert(u, __int128_t{-30}));
    REQUIRE(u == 30u);

    if constexpr ((piranha::detail::limits_max<__int128_t>) > std::numeric_limits<int>::max()
                  && (piranha::detail::limits_max<__uint128_t>) > std::numeric_limits<unsigned>::max()) {
        REQUIRE(!piranha::safe_convert(n, piranha::detail::limits_max<__int128_t>));
        REQUIRE(!piranha::safe_convert(u, piranha::detail::limits_max<__uint128_t>));
    }

#endif

    REQUIRE(piranha::is_safely_convertible_v<int &&, int &>);
    REQUIRE(piranha::is_safely_convertible_v<int &, int &>);
    REQUIRE(piranha::is_safely_convertible_v<const int &, int &>);
    REQUIRE(!piranha::is_safely_convertible_v<double &, int &>);
    REQUIRE(!piranha::is_safely_convertible_v<void, void>);
    REQUIRE(!piranha::is_safely_convertible_v<int &&, int>);
    REQUIRE(!piranha::is_safely_convertible_v<int &&, const int &>);
    REQUIRE(!piranha::is_safely_convertible_v<int, void>);
    REQUIRE(!piranha::is_safely_convertible_v<void, int>);
    REQUIRE(!piranha::is_safely_convertible_v<void, void>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::is_safely_convertible_v<int &&, __int128_t &>);
    REQUIRE(piranha::is_safely_convertible_v<int &, __uint128_t &>);
    REQUIRE(!piranha::is_safely_convertible_v<int &&, const __int128_t &>);
    REQUIRE(!piranha::is_safely_convertible_v<int &, __uint128_t &&>);
    REQUIRE(piranha::is_safely_convertible_v<__int128_t &&, int &>);
    REQUIRE(piranha::is_safely_convertible_v<__uint128_t &, int &>);
    REQUIRE(!piranha::is_safely_convertible_v<__int128_t &&, const int &>);
    REQUIRE(!piranha::is_safely_convertible_v<__uint128_t &, int &&>);
#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::SafelyConvertible<int &&, int &>);
    REQUIRE(piranha::SafelyConvertible<int &, int &>);
    REQUIRE(piranha::SafelyConvertible<const int &, int &>);
    REQUIRE(!piranha::SafelyConvertible<double &, int &>);
    REQUIRE(!piranha::SafelyConvertible<void, void>);
    REQUIRE(!piranha::SafelyConvertible<int &&, int>);
    REQUIRE(!piranha::SafelyConvertible<int &&, const int &>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::SafelyConvertible<int &&, __int128_t &>);
    REQUIRE(piranha::SafelyConvertible<int &, __uint128_t &>);
    REQUIRE(!piranha::SafelyConvertible<int &&, const __int128_t &>);
    REQUIRE(!piranha::SafelyConvertible<int &, __uint128_t &&>);
    REQUIRE(piranha::SafelyConvertible<__int128_t &&, int &>);
    REQUIRE(piranha::SafelyConvertible<__uint128_t &, int &>);
    REQUIRE(!piranha::SafelyConvertible<__int128_t &&, const int &>);
    REQUIRE(!piranha::SafelyConvertible<__uint128_t &, int &&>);
#endif
#endif
}

TEST_CASE("safe_convert_mppp_integer")
{
    using int_t = mppp::integer<1>;

    int n;
    REQUIRE(piranha::safe_convert(n, int_t{10}));
    REQUIRE(n == 10);
    REQUIRE(piranha::safe_convert(n, int_t{-10}));
    REQUIRE(n == -10);
    REQUIRE(!piranha::safe_convert(n, int_t{std::numeric_limits<int>::max()} + 1));
    REQUIRE(n == -10);
    REQUIRE(!piranha::safe_convert(n, int_t{std::numeric_limits<int>::min()} - 1));
    REQUIRE(n == -10);

    int_t out;
    REQUIRE(piranha::safe_convert(out, 10));
    REQUIRE(out == 10);
    REQUIRE(piranha::safe_convert(out, -10000l));
    REQUIRE(out == -10000l);

#if defined(PIRANHA_HAVE_GCC_INT128)

    __int128_t i128;
    __uint128_t ui128;
    REQUIRE(piranha::safe_convert(i128, int_t{45}));
    REQUIRE(i128 == 45);
    REQUIRE(piranha::safe_convert(i128, int_t{-45}));
    REQUIRE(i128 == -45);
    REQUIRE(piranha::safe_convert(ui128, int_t{45}));
    REQUIRE(ui128 == 45u);
    REQUIRE(!piranha::safe_convert(ui128, int_t{-44}));
    REQUIRE(ui128 == 45u);
    REQUIRE(piranha::safe_convert(out, __int128_t{33}));
    REQUIRE(out == 33);
    REQUIRE(piranha::safe_convert(out, __int128_t{-33}));
    REQUIRE(out == -33);
    REQUIRE(piranha::safe_convert(out, __uint128_t{32}));
    REQUIRE(out == 32);
    REQUIRE(!piranha::safe_convert(i128, int_t{piranha::detail::limits_max<__int128_t>} + 1));
    REQUIRE(!piranha::safe_convert(ui128, int_t{piranha::detail::limits_max<__uint128_t>} + 1));

#endif

    REQUIRE(piranha::is_safely_convertible_v<int &&, int_t &>);
    REQUIRE(piranha::is_safely_convertible_v<int &, int_t &>);
    REQUIRE(piranha::is_safely_convertible_v<const int &, int_t &>);
    REQUIRE(!piranha::is_safely_convertible_v<double &, int_t &>);
    REQUIRE(!piranha::is_safely_convertible_v<int &&, int_t>);
    REQUIRE(!piranha::is_safely_convertible_v<int &&, const int_t &>);
    REQUIRE(piranha::is_safely_convertible_v<int_t &&, int &>);
    REQUIRE(piranha::is_safely_convertible_v<int_t &, int &>);
    REQUIRE(piranha::is_safely_convertible_v<const int_t &, int &>);
    REQUIRE(!piranha::is_safely_convertible_v<int_t &, double &>);
    REQUIRE(!piranha::is_safely_convertible_v<int_t &&, int>);
    REQUIRE(!piranha::is_safely_convertible_v<int_t &&, const int &>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::is_safely_convertible_v<int_t &&, __int128_t &>);
    REQUIRE(piranha::is_safely_convertible_v<int_t &, __uint128_t &>);
    REQUIRE(!piranha::is_safely_convertible_v<int_t &&, const __int128_t &>);
    REQUIRE(!piranha::is_safely_convertible_v<int_t &, __uint128_t &&>);
    REQUIRE(piranha::is_safely_convertible_v<__int128_t &&, int_t &>);
    REQUIRE(piranha::is_safely_convertible_v<__uint128_t &, int_t &>);
    REQUIRE(!piranha::is_safely_convertible_v<__int128_t &&, const int_t &>);
    REQUIRE(!piranha::is_safely_convertible_v<__uint128_t &, int_t &&>);
#endif
}

// Test the customisation machinery.

// A new type.
struct foo0 {
};

// Customise piranha::safe_convert() for foo0.
namespace piranha::customisation
{

template <typename T, typename U>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, foo0> &&SameCvr<U, foo0> inline constexpr auto safe_convert<T, U>
#else
inline constexpr auto safe_convert<T, U, std::enable_if_t<is_same_cvr_v<T, foo0> && is_same_cvr_v<U, foo0>>>
#endif
    = [](auto &&, auto &&) constexpr noexcept
{
    return true;
};

} // namespace piranha::customisation

TEST_CASE("safe_convert_custom")
{
    REQUIRE(!piranha::is_safely_convertible_v<foo0, int>);
    REQUIRE(!piranha::is_safely_convertible_v<int, foo0>);
    REQUIRE(piranha::is_safely_convertible_v<foo0, foo0>);

    REQUIRE(piranha::safe_convert(foo0{}, foo0{}));
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
    REQUIRE(piranha::is_safely_convertible_v<std::string, std::string &>);
    REQUIRE(piranha::is_safely_convertible_v<const std::string &, std::string &>);
    REQUIRE(piranha::is_safely_convertible_v<std::string &&, std::string &>);
    REQUIRE(!piranha::is_safely_convertible_v<std::string &&, const std::string &>);
    REQUIRE(piranha::is_safely_convertible_v<std::string &&, std::string &&>);

    REQUIRE(piranha::is_safely_convertible_v<foo1, foo1 &>);
    REQUIRE(piranha::is_safely_convertible_v<const foo1 &, foo1 &>);
    REQUIRE(piranha::is_safely_convertible_v<foo1 &&, foo1 &>);
    REQUIRE(!piranha::is_safely_convertible_v<foo1 &&, const foo1 &>);
    REQUIRE(piranha::is_safely_convertible_v<foo1 &&, foo1 &&>);

    REQUIRE(!piranha::is_safely_convertible_v<foo2, foo2 &>);
    REQUIRE(!piranha::is_safely_convertible_v<const foo2 &, foo2 &>);
    REQUIRE(!piranha::is_safely_convertible_v<foo2 &&, foo2 &>);
    REQUIRE(!piranha::is_safely_convertible_v<foo2 &&, const foo2 &>);
    REQUIRE(!piranha::is_safely_convertible_v<foo2 &&, foo2 &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::SafelyConvertible<std::string, std::string &>);
    REQUIRE(piranha::SafelyConvertible<const std::string &, std::string &>);
    REQUIRE(piranha::SafelyConvertible<std::string &&, std::string &>);
    REQUIRE(!piranha::SafelyConvertible<std::string &&, const std::string &>);
    REQUIRE(piranha::SafelyConvertible<std::string &&, std::string &&>);

    REQUIRE(piranha::SafelyConvertible<foo1, foo1 &>);
    REQUIRE(piranha::SafelyConvertible<const foo1 &, foo1 &>);
    REQUIRE(piranha::SafelyConvertible<foo1 &&, foo1 &>);
    REQUIRE(!piranha::SafelyConvertible<foo1 &&, const foo1 &>);
    REQUIRE(piranha::SafelyConvertible<foo1 &&, foo1 &&>);

    REQUIRE(!piranha::SafelyConvertible<foo2, foo2 &>);
    REQUIRE(!piranha::SafelyConvertible<const foo2 &, foo2 &>);
    REQUIRE(!piranha::SafelyConvertible<foo2 &&, foo2 &>);
    REQUIRE(!piranha::SafelyConvertible<foo2 &&, const foo2 &>);
    REQUIRE(!piranha::SafelyConvertible<foo2 &&, foo2 &&>);
#endif

    std::string s = "hello";
    REQUIRE(piranha::safe_convert(s, std::string{"world"}));
    REQUIRE(s == "world");
}
