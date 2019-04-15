// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_CONTAINERS_HASH_MAP_HPP
#define PIRANHA_CONTAINERS_HASH_MAP_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/simd.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename K, typename T, Hash<K> H = ::std::hash<K>>
#else
template <typename K, typename T, typename H = ::std::hash<K>, typename = ::std::enable_if_t<is_hash_v<H, K>>>
#endif
class hash_map
{
    static constexpr unsigned group_size =
#if defined(PIRANHA_HAVE_AVX2)
        32u
#elif defined(PIRANHA_HAVE_SSE2)
        16u
#else
        1u
#endif
        ;
    static_assert(group_size > 0u && (group_size & (group_size - 1u)) == 0u);

public:
    using value_type = ::std::pair<const K, T>;
    using size_type = ::std::size_t;
    constexpr hash_map() : hash_map(H{}) {}
    constexpr explicit hash_map(const H &h) : m_pack(nullptr, h), m_size(0), m_groups(0) {}
    ~hash_map()
    {
        const auto p = ptr();
        if (p) {

        } else {
            assert(m_groups == 0u);
            assert(m_size == 0u);
        }
    }

private:
    // Helper to compute the max possible size for a table.
    static constexpr size_type compute_max_size()
    {
        // The max number of elements that can be stored taking into account that
        // only part of the hash value is used for modulo reduction (the upper bits
        // are used in the metadata).
        // NOTE: the number of bits in the metadata is the number of bits
        // in unsigned char minus 1 bit (that is, 7 bits).
        constexpr auto max_size_hash
            = static_cast<size_type>(::std::size_t(1) << (::std::numeric_limits<::std::size_t>::digits
                                                          - (::std::numeric_limits<unsigned char>::digits - 1)));

        // The max size in bytes. Need space for the table elements, for the metadata
        // and padding at the end for SIMD loading of the metadata at the end
        // of the table:
        // max_size_bytes * sizeof(value_type) + max_size_bytes + (group_size - 1u);
        // This must not be larger than the max of std::size_t.
        constexpr auto size_t_max = ::std::numeric_limits<::std::size_t>::max();
        static_assert(sizeof(value_type) < size_t_max);
        static_assert(group_size > 0u && group_size <= 256u);
        constexpr auto max_size_bytes
            = static_cast<size_type>((size_t_max - group_size + 1u) / (sizeof(value_type) + 1u));

        constexpr auto candidate = ::std::min(max_size_bytes, max_size_hash);

        // In the table, we store 2**n groups. We need to figure
        // out the highest n_max such that 2**n_max <= candidate.
        unsigned tmp = 1;
        size_type tot = group_size_bytes;
        while (true) {

            group_size_bytes * 2u;
            ++tmp;
        }

        return candidate;
    }
    static constexpr auto max_size_impl = compute_max_size();

public:
    size_type max_size() const noexcept
    {
        return max_size_impl;
    }

private:
    static bool cmp_keys(const K &k1, const K &k2)
    {
        return static_cast<bool>(k1 == k2);
    }
    unsigned char *ptr()
    {
        return ::std::get<0>(m_pack).get();
    }
    const unsigned char *ptr() const
    {
        return ::std::get<0>(m_pack).get();
    }

private:
    // Pack pointer and hasher in a tuple
    // to exploit likely EBO.
    ::std::tuple<::std::unique_ptr<unsigned char[]>, H> m_pack;
    // The number of elements currently
    // stored in the table.
    size_type m_size;
    // The base-2 log of the number of groups currently
    // allocated in the table.
    // NOTE: for an empty table, this will
    // be zero.
    unsigned m_groups;
};

} // namespace piranha

#endif