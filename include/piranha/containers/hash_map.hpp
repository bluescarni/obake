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

public:
    using value_type = ::std::pair<const K, T>;
    using size_type = ::std::size_t;
    constexpr hash_map() : hash_map(H{}) {}
    constexpr explicit hash_map(const H &hash) : m_pack(nullptr, hash), m_size(0), m_groups(0) {}

private:
    const auto &hasher() const
    {
        return ::std::get<1>(m_pack);
    }

private:
    // Pack pointer and hasher in a tuple
    // to exploit likely EBO for H.
    using pack_t = ::std::tuple<unsigned char *, H>;

    pack_t m_pack;
    // The number of elements currently
    // stored in the table.
    size_type m_size;
    // The number of groups currently
    // allocated in the table.
    size_type m_groups;
};

} // namespace piranha

#endif