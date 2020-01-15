// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cassert>
#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/type_traits.hpp>

namespace obake::detail
{

namespace
{

template <typename T>
::std::string to_string_unsigned_integral_impl(const T &n_)
{
    // Operate on a copy.
    auto n(n_);

    // NOTE: digits10 means:
    // """
    // Any number with this many significant decimal digits
    // can be converted to a value of type T and back to decimal
    // form, without change due to rounding or overflow.
    // """
    // That is, the type T may be able to represent numbers
    // with digits10 + 1 number of decimal digits, although
    // not all of them.
    //
    // For instance, for a 16bit unsigned the max value
    // is 65535, which consists of 5 decimal digits. Hence,
    // digits10 will be 4 because a 16bit unsigned can
    // represent all numbers with 4 decimal digits but only
    // some numbers with 5 decimal digits.
    static_assert(static_cast<unsigned>(limits_digits10<T>) < limits_max<::std::size_t>, "Overflow error.");
    char buffer[static_cast<::std::size_t>(limits_digits10<T>) + 1u];

    // Write the reversed base-10 representation into the buffer.
    ::std::size_t len = 0;
    while (n) {
        buffer[len++] = static_cast<char>('0' + n % 10u);
        n /= 10u;
    }

    // Construct the return value, special-casing zero.
    if (len) {
        assert(len <= sizeof(buffer));
        return ::std::string(::std::make_reverse_iterator(buffer + len), ::std::make_reverse_iterator(buffer));
    } else {
        return "0";
    }
}

// NOTE: MSVC warns about using the negation operator
// on unsigned integrals.
#if defined(_MSC_VER)

#pragma warning(push)
#pragma warning(disable : 4146)

#endif

template <typename T>
::std::string to_string_signed_integral_impl(const T &n_)
{
    // Operate on the absolute value.
    const bool negative = n_ < T(0);

    // In order to compute the abs of negative values, we use
    // the usual trick of casting to unsigned
    // and then negating. We have to take care
    // however, because if T is a signed integral
    // whose rank is less than int, integral promotions
    // could bite us.
    auto n = [negative, &n_]() {
        // Check if T is subject to integral promotions.
        // NOTE: MSVC seems to have a bug, returning a const
        // qualified type from the builtin unary operator+() if
        // the operand is const:
        // https://www.reddit.com/r/cpp/comments/bpirh0/is_this_an_msvc_bug_or_implementationdefined/
        // Hence, we forcibly remove constness.
        if constexpr (::std::is_same_v<::std::remove_const_t<decltype(+n_)>, T>) {
            // No integral promotions, compute abs using
            // the original type and its unsigned counterpart.
            const auto un = static_cast<make_unsigned_t<T>>(n_);
            return negative ? -un : un;
        } else {
            // Integral promotions: compute abs via unsigned, then cast
            // back to the unsigned counterpart of T.
            const auto un = static_cast<unsigned>(n_);
            return static_cast<make_unsigned_t<T>>(negative ? -un : un);
        }
    }();

    // See the explanation above. We need an extra
    // slot for the minus sign.
    static_assert(static_cast<unsigned>(limits_digits10<T>) < limits_max<::std::size_t> - 1u, "Overflow error.");
    char buffer[static_cast<::std::size_t>(limits_digits10<T>) + 2u];

    // Write the reversed base-10 representation into the buffer.
    ::std::size_t len = 0;
    while (n) {
        buffer[len++] = static_cast<char>('0' + n % 10u);
        n /= 10u;
    }

    // Construct the return value, special-casing zero.
    if (len) {
        if (negative) {
            // Add the sign if n is negative.
            buffer[len++] = '-';
        }
        assert(len <= sizeof(buffer));
        return ::std::string(::std::make_reverse_iterator(buffer + len), ::std::make_reverse_iterator(buffer));
    } else {
        return "0";
    }
}

#if defined(_MSC_VER)

#pragma warning(pop)

#endif

template <typename T>
::std::string to_string_integral_impl(const T &n)
{
    if constexpr (is_signed_v<T>) {
        return ::obake::detail::to_string_signed_integral_impl(n);
    } else {
        return ::obake::detail::to_string_unsigned_integral_impl(n);
    }
}

} // namespace

#define OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(type)                                                                       \
    template <>                                                                                                        \
    ::std::string to_string(const type &n)                                                                             \
    {                                                                                                                  \
        return ::obake::detail::to_string_integral_impl(n);                                                            \
    }

// Char types.
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(char)
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(signed char)
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(unsigned char)

// Shorts.
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(short)
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(unsigned short)

// Ints.
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(int)
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(unsigned)

// Longs.
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(long)
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(unsigned long)

// Longlongs.
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(long long)
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(unsigned long long)

#if defined(OBAKE_HAVE_GCC_INT128)

// 128bit ints.
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(__int128_t)
OBAKE_IMPLEMENT_TO_STRING_INTEGRAL(__uint128_t)

#endif

#undef OBAKE_IMPLEMENT_TO_STRING_INTEGRAL

} // namespace obake::detail
