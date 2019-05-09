// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_SERIES_HPP
#define PIRANHA_SERIES_HPP

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <vector>

#include <absl/container/flat_hash_map.h>

#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <piranha/config.hpp>
#include <piranha/hash.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

template <typename T>
using is_key = ::std::conjunction<::std::is_destructible<T>, is_hashable<T>>;

template <typename T>
inline constexpr bool is_key_v = is_key<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Key = is_key_v<T>;

#endif

template <typename T>
using is_cf = ::std::conjunction<::std::is_destructible<T>, is_zero_testable<T>>;

template <typename T>
inline constexpr bool is_cf_v = is_cf<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Cf = is_cf_v<T>;

#endif

namespace detail
{

// A small hashing wrapper for keys. It accomplishes the task
// of forcing the evaluation of a key through const reference,
// so that, in the Key requirements, we can request hashability
// through const lvalue ref.
struct key_hasher {
    template <typename K>
    constexpr ::std::size_t operator()(const K &k) const noexcept(noexcept(::piranha::hash(k)))
    {
        return ::piranha::hash(k);
    }
};

} // namespace detail

#if defined(PIRANHA_HAVE_CONCEPTS)
template <Cf C, Key K, typename Tag>
#else
template <typename C, typename K, typename Tag,
          typename = ::std::enable_if_t<::std::conjunction_v<is_cf<C>, is_key<K>>>>
#endif
class series
{
public:
    using container_t = ::std::vector<::absl::flat_hash_map<K, C, detail::key_hasher>>;

private:
    template <typename T>
    class iterator_impl : public ::boost::iterator_facade<iterator_impl<T>, T, ::boost::forward_traversal_tag>
    {
        template <typename U>
        friend class iterator_impl;
        friend class ::boost::iterator_core_access;

        using container_ptr_t = ::std::conditional_t<::std::is_const_v<T>, const container_t *, container_t *>;
        using idx_t = typename container_t::size_type;
        using local_it_t = ::std::conditional_t<::std::is_const_v<T>, typename container_t::value_type::const_iterator,
                                                typename container_t::value_type::iterator>;

    public:
        iterator_impl() : m_container_ptr(nullptr), m_idx(0), m_local_it{} {}
        explicit iterator_impl(container_ptr_t *container_ptr, idx_t idx, local_it_t local_it)
            : m_container_ptr(container_ptr), m_idx(idx), m_local_it(local_it)
        {
        }

    private:
        void increment()
        {
            // Must be pointing to something.
            assert(m_container_ptr != nullptr);
            // Cannot be already at the end of the container.
            assert(m_idx < m_container_ptr->size());
            // The current table cannot be empty.
            assert(!m_container_ptr[m_idx].empty());
            // The local iterator cannot be pointing
            // at the end of the current table.
            assert(m_local_it != m_container_ptr[m_idx].end());
            // Move to the next item in the current table.
            ++m_local_it;
            if (m_local_it == m_container_ptr[m_idx].end()) {
                // We reached the end of the current table.
                // Keep bumping up m_idx until we either
                // arrive in a non-empty table, or we reach
                // the end of the container.
                const auto c_size = m_container_ptr->size();
                while (true) {
                    ++m_idx;
                    if (m_idx == c_size) {
                        // End of the container, reset
                        // m_local_it to def-cted and
                        // exit.
                        m_local_it = local_it_t{};
                        break;
                    } else if (!m_container_ptr[m_idx].empty()) {
                        // Local non-empty table, set
                        // m_local_it to the beginning and
                        // exit.
                        m_local_it = m_container_ptr[m_idx].begin();
                        break;
                    }
                }
            }
        }
        bool equal(const iterator_impl &other) const
        {
            // Can compare only iterators referring to the same container.
            assert(m_container_ptr != nullptr && m_container_ptr == other.m_container_ptr);
            return (m_idx == other.m_idx && m_local_it == other.m_local_it);
        }

    private:
        container_ptr_t m_container_ptr;
        idx_t m_idx;
        local_it_t m_local_it;
    };

private:
    container_t m_container;
};

namespace detail
{

template <typename T>
struct is_series_impl : ::std::false_type {
};

template <typename C, typename K, typename Tag>
struct is_series_impl<series<C, K, Tag>> : ::std::true_type {
};

} // namespace detail

template <typename T>
using is_cvr_series = detail::is_series_impl<remove_cvref_t<T>>;

template <typename T>
inline constexpr bool is_cvr_series_v = is_cvr_series<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CvrSeries = is_cvr_series_v<T>;

#endif

namespace detail
{

template <typename>
struct series_cf_t_impl {
};

template <typename C, typename K, typename Tag>
struct series_cf_t_impl<series<C, K, Tag>> {
    using type = C;
};

} // namespace detail

template <typename T>
using series_cf_t = typename detail::series_cf_t_impl<::std::remove_cv_t<T>>::type;

namespace customisation::internal
{

template <typename T, typename U>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires CvrSeries<T> &&Integral<::std::remove_reference_t<U>> inline constexpr auto pow<T, U>
#else
inline constexpr auto
    pow<T, U, ::std::enable_if_t<::std::conjunction_v<is_cvr_series<T>, is_integral<::std::remove_reference_t<U>>>>>
#endif
    = [](auto &&, auto &&) constexpr
{
    return 0;
};

} // namespace customisation::internal

} // namespace piranha

#endif
