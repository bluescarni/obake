// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_CONTAINERS_HASH_MAP_HPP
#define PIRANHA_CONTAINERS_HASH_MAP_HPP

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
    using KE = ::std::equal_to<K>;

public:
    using value_type = ::std::pair<const K, T>;
    using size_type = ::std::size_t;
    constexpr hash_map() : hash_map(H{}) {}
    constexpr explicit hash_map(const H &h) : m_pack(nullptr, h, KE{}), m_size(0), m_groups(0) {}
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
    static constexpr size_type compute_max_size()
    {
        // The max number of elements that can be stored taking into account that
        // only part of the hash value is used for modulo reduction (the upper bits
        // are used in the metadata).
        // NOTE: here we take advantage of the fact that
        // size_type == std::size_t, the same type used for hash values.
        // NOTE: the number of bits in the metadata is the number of bits
        // in unsigned char minus 1 bit (that is, 7 bits).
        constexpr auto max_size_hash
            = static_cast<size_type>(size_type(1) << (::std::numeric_limits<size_type>::digits
                                                      - (::std::numeric_limits<unsigned char>::digits - 1)));

        // The max size in bytes: max_size_hash values, max_size_hash metadata bytes,
        // extra padding at the end for SIMD loading of the metadata at the
        // end of the table:
        // max_size_hash * sizeof(value_type) + max_size_hash + (group_size - 1u);
        // This must not be larger than the max of std::size_t.
        constexpr auto candidate = []() {
            constexpr auto size_t_max = ::std::numeric_limits<size_type>::max();
            static_assert(sizeof(value_type) < size_t_max);
            static_assert(group_size > 0u && group_size <= 256u);
            if constexpr (max_size_hash <= (size_t_max - group_size + 1u) / (sizeof(value_type) + 1u)) {
                return max_size_hash;
            } else {
                return static_cast<size_type>((size_t_max - group_size + 1u) / (sizeof(value_type) + 1u));
            }
        }();

        return candidate;
    }
    static constexpr auto max_size_impl = compute_max_size();

public:
    size_type max_size() const noexcept
    {
        return max_size_impl;
    }

private:
    bool cmp_keys(const K &k1, const K &k2) const
    {
        return static_cast<bool>(::std::get<2>(m_pack)(k1, k2));
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
    // Pack pointer, hasher and comparator in a tuple
    // to exploit likely EBO.
    ::std::tuple<::std::unique_ptr<unsigned char[]>, H, KE> m_pack;
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