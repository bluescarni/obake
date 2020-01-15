// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

// xoroshiro128+ implementation, slightly adapted from:
// http://vigna.di.unimi.it/xorshift/xoroshiro128plus.c
// See also:
// https://en.wikipedia.org/wiki/Xoroshiro128%2B
// Original copyright notice follows:

/*  Written in 2016-2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

#ifndef OBAKE_DETAIL_XOROSHIRO128_PLUS_HPP
#define OBAKE_DETAIL_XOROSHIRO128_PLUS_HPP

#include <cstdint>

#include <obake/detail/limits.hpp>
#include <obake/type_traits.hpp>

namespace obake::detail
{

// NOTE: constexpr implementation, thus usable at compile-time.
struct xoroshiro128_plus {
    static constexpr ::std::uint64_t rotl(const ::std::uint64_t &x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }

    // Constructor from seed state (2 64-bit values).
    constexpr explicit xoroshiro128_plus(const ::std::uint64_t &s0, const ::std::uint64_t &s1) : m_state{s0, s1} {}

    // Compute the next 64-bit value in the sequence.
    constexpr ::std::uint64_t next()
    {
        const auto s0 = m_state[0];
        auto s1 = m_state[1];
        const auto result = s0 + s1;

        s1 ^= s0;
        m_state[0] = xoroshiro128_plus::rotl(s0, 24) ^ s1 ^ (s1 << 16);
        m_state[1] = xoroshiro128_plus::rotl(s1, 37);

        return result;
    }

    // Generate an integral value of type Int
    // with uniform probability within the whole range.
    template <typename Int>
    constexpr Int random()
    {
        static_assert(is_integral_v<Int>);

        // NOTE: generate using the unsigned counterpart,
        // cast to Int at the end.
        using uint_t = make_unsigned_t<Int>;
        // Total number of bits in the *unsigned* counterpart.
        constexpr auto tot_nbits = limits_digits<uint_t>;

        // Generate the first 64 bits.
        auto u_retval = static_cast<uint_t>(next());

        if constexpr (tot_nbits > 64) {
            // If the bit width is larger than 64, generate
            // the remaining bits in chunks of 64.
            // NOTE: i = number of bits yet to be generated.
            for (auto i = tot_nbits - 64; i > 0; i -= 64) {
                u_retval <<= 64;
                u_retval += static_cast<uint_t>(next());
            }
        }

        // Cast back to the original type.
        // NOTE: in case Int is signed, this operation is
        // implementation-defined up to C++20, and from C++20
        // this follows the rules of two's complement arithmetic.
        // In practice, all implementations follow two's complement
        // even before C++20.
        return static_cast<Int>(u_retval);
    }

    // Provide also an interface compatible with the UniformRandomBitGenerator concept:
    // https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator
    using result_type = ::std::uint64_t;
    static constexpr result_type min()
    {
        return 0;
    }
    static constexpr result_type max()
    {
        return limits_max<result_type>;
    }
    constexpr result_type operator()()
    {
        return next();
    }

    ::std::uint64_t m_state[2];
};

} // namespace obake::detail

#endif
