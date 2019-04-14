// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_CONTAINERS_HASH_MAP_HPP
#define PIRANHA_CONTAINERS_HASH_MAP_HPP

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
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
    static constexpr unsigned group_size = 16u;
    static_assert(group_size > 0u && (group_size & (group_size - 1u)) == 0u);
    using KE = ::std::equal_to<K>;

public:
    using value_type = ::std::pair<const K, T>;
    using size_type = ::std::size_t;
    constexpr hash_map() : hash_map(H{}) {}
    constexpr explicit hash_map(const H &h) : m_pack(nullptr, h, KE{}), m_size(0), m_groups(0) {}

private:
    bool cmp_keys(const K &k1, const K &k2) const
    {
        return static_cast<bool>(::std::get<2>(m_pack)(k1, k2));
    }

private:
    // Pack pointer, hasher and comparator in a tuple
    // to exploit likely EBO.
    ::std::tuple<unsigned char *, H, KE> m_pack;
    // The number of elements currently
    // stored in the table.
    size_type m_size;
    // The number of groups currently
    // allocated in the table.
    size_type m_groups;
};

} // namespace piranha

#endif