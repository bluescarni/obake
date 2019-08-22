// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdint>

#include <piranha/config.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/k_packing.hpp>
#include <piranha/type_traits.hpp>

namespace piranha::detail
{

namespace
{

// xoroshiro128+ implementation, slightly adapted from:
// http://vigna.di.unimi.it/xorshift/xoroshiro128plus.c
// See also:
// https://en.wikipedia.org/wiki/Xoroshiro128%2B
struct xoroshiro128_plus {
    static constexpr ::std::uint64_t rotl(const ::std::uint64_t &x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }
    constexpr explicit xoroshiro128_plus(const ::std::uint64_t &s0, const ::std::uint64_t &s1) : m_state{s0, s1} {}
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
    // Generated an integral value of type Int
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
        return static_cast<Int>(u_retval);
    }

    ::std::uint64_t m_state[2];
};

[[maybe_unused]] constexpr auto val1 = xoroshiro128_plus{1, 0}.random<int>();
[[maybe_unused]] constexpr auto val2 = xoroshiro128_plus{2, 0}.random<unsigned>();
[[maybe_unused]] constexpr auto val3 = xoroshiro128_plus{3, 0}.random<long>();
[[maybe_unused]] constexpr auto val4 = xoroshiro128_plus{4, 0}.random<unsigned long>();
[[maybe_unused]] constexpr auto val5 = xoroshiro128_plus{5, 0}.random<long long>();
[[maybe_unused]] constexpr auto val6 = xoroshiro128_plus{6, 0}.random<unsigned long long>();

#if defined(PIRANHA_HAVE_GCC_INT128)

[[maybe_unused]] constexpr auto val7 = xoroshiro128_plus{7, 0}.random<__int128_t>();
[[maybe_unused]] constexpr auto val8 = xoroshiro128_plus{8, 0}.random<__uint128_t>();

#endif

} // namespace

} // namespace piranha::detail
