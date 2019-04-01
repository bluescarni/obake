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

#include <piranha/detail/to_string.hpp>

#endif

namespace piranha::detail
{

#if defined(PIRANHA_HAVE_GCC_INT128)

template <>
::std::string to_string(const __uint128_t &n_)
{
    // Operate on a copy.
    auto n(n_);

    // NOTE: the maximum unsigned value
    // is 39 digits long in base 10.
    char buffer[39];

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

template <>
::std::string to_string(const __int128_t &n_)
{
    // Operate on the absolute value.
    const bool negative = n_ < 0;
    auto n = negative ? -static_cast<__uint128_t>(n_) : static_cast<__uint128_t>(n_);

    // NOTE: the largest value (in terms of base-10 digits)
    // is -2**127, which is 40 digits long.
    char buffer[40];

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

#endif

} // namespace piranha::detail
