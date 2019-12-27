// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POWER_SERIES_TPS_HPP
#define OBAKE_POWER_SERIES_TPS_HPP

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/container/flat_map.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/variant/variant.hpp>

#include <obake/key/key_degree.hpp>
#include <obake/key/key_p_degree.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/symbols.hpp>

namespace obake
{

namespace power_series
{

// Forward declaration.
// TODO type requirements
template <typename K, typename C, typename = void>
class tps;

namespace detail
{

// Struct to represent the absence
// of truncation in a power series.
struct no_truncation {
};

} // namespace detail

// TODO type requirements.
template <typename K, typename C, typename>
class tps
{
public:
    // Useful typedefs.
    using poly_t = polynomial<K, C>;
    using degree_t = ::obake::detail::key_degree_t<const K &>;
    // TODO assert equal to degree_t.
    // using p_degree_t = ::obake::detail::key_p_degree_t<const K &>;
    // The truncation setting type.
    using trunc_t = ::boost::variant<detail::no_truncation, degree_t, ::std::tuple<degree_t, symbol_set>>;

private:
    // NOTE: the default comparator of flat_map
    // uses std::less, which casts its
    // arguments to const references.
    using container_t = ::boost::container::flat_map<degree_t, poly_t>;

    // Size type of the poly container.
    using c_size_t = typename container_t::size_type;

    // A small helper to select the (const) iterator of poly_t, depending on whether
    // T is const or not. Used in the iterator implementation below.
    template <typename T>
    using local_it_t
        = ::std::conditional_t<::std::is_const_v<T>, typename poly_t::const_iterator, typename poly_t::iterator>;

    // NOTE: this is mostly taken from:
    // https://www.boost.org/doc/libs/1_70_0/libs/iterator/doc/iterator_facade.html
    template <typename T>
    class iterator_impl
        : public ::boost::iterator_facade<iterator_impl<T>, T, ::boost::forward_traversal_tag, T &,
                                          // Fetch the difference type from the underlying local iterator.
                                          typename ::std::iterator_traits<local_it_t<T>>::difference_type>
    {
        // Select the type of the pointer to the segmented table
        // with the correct constness.
        using container_ptr_t = ::std::conditional_t<::std::is_const_v<T>, const container_t *, container_t *>;

    public:
        // Default constructor.
        // NOTE: C++14 requires that all value-inited forward iterators
        // compare equal. This is guaranteed by this constructor, since
        // local_it_t is also a forward iterator which is default-inited.
        // https://en.cppreference.com/w/cpp/named_req/ForwardIterator
        iterator_impl() : m_container_ptr(nullptr), m_idx(0), m_local_it{} {}

        // Specialise the swap primitive.
        friend void swap(iterator_impl &it1, iterator_impl &it2) noexcept
        {
            using ::std::swap;

            swap(it1.m_container_ptr, it2.m_container_ptr);
            swap(it1.m_idx, it2.m_idx);
            // NOTE: all iterators are required to be swappable,
            // so this must always be supported by the local iterator
            // type.
            swap(it1.m_local_it, it2.m_local_it);
        }

    private:
        container_ptr_t m_container_ptr;
        c_size_t m_idx;
        local_it_t<T> m_local_it;
    };

private:
    symbol_set m_symbol_set;
    container_t m_container;
};

} // namespace power_series

template <typename K, typename C>
using tps = power_series::tps<K, C>;

} // namespace obake

#endif
