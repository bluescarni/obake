// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/config.hpp>

// Don't include any header if we don't have 128-bit integer support.
#if defined(PIRANHA_HAVE_GCC_INT128)

#include <cassert>
#include <cstddef>
#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>

#include <piranha/detail/limits.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/type_traits.hpp>

#endif

namespace piranha::detail
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
    static_assert(static_cast<unsigned>(limits_digits10<T>) < ::std::get<1>(limits_minmax<::std::size_t>),
                  "Overflow error.");
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
        if constexpr (::std::is_same_v<decltype(+n_), T>) {
            // No integral promotions, compute abs using
            // the original type and its unsigned counterpart.
            return negative ? -static_cast<make_unsigned_t<T>>(n_) : static_cast<make_unsigned_t<T>>(n_);
        } else {
            // Integral promotions: cast to int, compute abs, cast
            // back to the unsigned counterpart of T.
            return static_cast<make_unsigned_t<T>>(negative ? -static_cast<unsigned>(static_cast<int>(n_))
                                                            : static_cast<unsigned>(static_cast<int>(n_)));
        }
    }();

    // See the explanation above. We need an extra
    // slot for the minus sign.
    static_assert(static_cast<unsigned>(limits_digits10<T>) < ::std::get<1>(limits_minmax<::std::size_t>) - 1u,
                  "Overflow error.");
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

template <typename T>
::std::string to_string_integral_impl(const T &n)
{
    if constexpr (is_signed_v<T>) {
        return ::piranha::detail::to_string_signed_integral_impl(n);
    } else {
        return ::piranha::detail::to_string_unsigned_integral_impl(n);
    }
}

} // namespace

#define PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(type)                                                                     \
    template <>                                                                                                        \
    ::std::string to_string(const type &n)                                                                             \
    {                                                                                                                  \
        return ::piranha::detail::to_string_integral_impl(n);                                                          \
    }

// Char types.
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(char)
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(signed char)
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(unsigned char)

// Shorts.
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(short)
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(unsigned short)

// Ints.
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(int)
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(unsigned)

// Longs.
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(long)
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(unsigned long)

// Longlongs.
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(long long)
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(unsigned long long)

#if defined(PIRANHA_HAVE_GCC_INT128)

// 128bit ints.
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(__int128_t)
PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL(__uint128_t)

#endif

#undef PIRANHA_IMPLEMENT_TO_STRING_INTEGRAL

} // namespace piranha::detail
