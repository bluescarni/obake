// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POWER_SERIES_TPS_HPP
#define OBAKE_POWER_SERIES_TPS_HPP

#include <cassert>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/container/flat_map.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/variant/variant.hpp>

#include <obake/config.hpp>
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
        // Make friends with boost's iterator machinery.
        friend class ::boost::iterator_core_access;
        // Make friends also with the tps class, as sometimes
        // we need to poke the internals of the iterator from there.
        friend class tps;

        // Select the type of the pointer to the container
        // with the correct constness.
        using container_ptr_t = ::std::conditional_t<::std::is_const_v<T>, const container_t *, container_t *>;

    public:
        // Default constructor.
        // NOTE: C++14 requires that all value-inited forward iterators
        // compare equal. This is guaranteed by this constructor, since
        // local_it_t is also a forward iterator which is default-inited.
        // https://en.cppreference.com/w/cpp/named_req/ForwardIterator
        iterator_impl() : m_container_ptr(nullptr), m_idx(0), m_local_it{} {}

        // Init with a pointer to the container and an index.
        // Used in the begin()/end() implementations.
        // NOTE: ensure m_local_it is default-inited, so it is in
        // a known state. This is also exploited in the implementation
        // of begin()/end().
        explicit iterator_impl(container_ptr_t container_ptr, c_size_t idx)
            : m_container_ptr(container_ptr), m_idx(idx), m_local_it{}
        {
        }

        // Implicit converting ctor from another specialisation. This is
        // used to construct a const iterator from a mutable one.
        template <typename U,
                  ::std::enable_if_t<
                      ::std::conjunction_v<
                          // NOTE: prevent competition with the
                          // copy constructor.
                          ::std::negation<::std::is_same<T, U>>,
                          ::std::is_constructible<container_ptr_t, const typename iterator_impl<U>::container_ptr_t &>,
                          ::std::is_constructible<local_it_t<T>, const local_it_t<U> &>>,
                      int> = 0>
        iterator_impl(const iterator_impl<U> &other)
            : m_container_ptr(other.m_container_ptr), m_idx(other.m_idx), m_local_it(other.m_local_it)
        {
        }

        // Default the copy/move ctors/assignment operators.
        iterator_impl(const iterator_impl &) = default;
        iterator_impl(iterator_impl &&) = default;
        iterator_impl &operator=(const iterator_impl &) = default;
        iterator_impl &operator=(iterator_impl &&) = default;

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
        void increment()
        {
            // Must be pointing to something.
            assert(m_container_ptr != nullptr);
            // Cannot be already at the end of the container.
            assert(m_idx < m_container_ptr->size());
            // The current poly cannot be empty.
            assert(!m_container_ptr->nth(m_idx)->second.empty());
            // The local iterator cannot be pointing
            // at the end of the current poly.
            assert(m_local_it != m_container_ptr->nth(m_idx)->second.end());

            // Move to the next item in the current poly.
            auto &c = *m_container_ptr;
            ++m_local_it;

            // NOTE: we expect to have many more elements per
            // poly than the total number of polys, hence
            // moving to the next poly should be a relatively
            // rare occurrence.
            if (obake_unlikely(m_local_it == c.nth(m_idx)->second.end())) {
                // We reached the end of the current poly.
                // Keep bumping m_idx until we either
                // arrive in a non-empty poly, or we reach
                // the end of the container.
                const auto c_size = c.size();
                while (true) {
                    ++m_idx;
                    if (m_idx == c_size) {
                        // End of the container, reset m_local_it to
                        // value-inited and exit.
                        // NOTE: this is important, because this is now an end()
                        // iterator and end() iterators contain a value-inited
                        // local iterator.
                        m_local_it = local_it_t<T>{};
                        break;
                    } else if (!c.nth(m_idx)->second.empty()) {
                        // The next non-empty poly was found.
                        // Set m_local_it to its beginning and exit.
                        m_local_it = c.nth(m_idx)->second.begin();
                        break;
                    }
                }
            }
        }

        // NOTE: templated in order to enable comparisons
        // between the const and mutable variants.
        template <typename U,
                  ::std::enable_if_t<is_equality_comparable_v<const local_it_t<T> &, const local_it_t<U> &>, int> = 0>
        bool equal(const iterator_impl<U> &other) const
        {
            // NOTE: comparison is defined either for singular iterators,
            // or for iterators referring to the same underlying sequence.
            //
            // Singular iterators are signalled by a null container pointer,
            // zero index and a singular local iterator. Thus, using this
            // comparison operator with two singular iterators will always
            // yield true, as required.
            //
            // Non-singular iterators must refer to the same container.
            // If they don't, we have UB (assertion failure in debug mode).
            assert(m_container_ptr == other.m_container_ptr);

            // NOTE: this is fine when comparing end() with itself,
            // as m_local_it as a unique representation for end
            // iterators.
            return (m_idx == other.m_idx && m_local_it == other.m_local_it);
        }

    private:
        container_ptr_t m_container_ptr;
        c_size_t m_idx;
        local_it_t<T> m_local_it;
    };

public:
    using iterator = iterator_impl<series_term_t<poly_t>>;
    using const_iterator = iterator_impl<const series_term_t<poly_t>>;

private:
    // Abstract out the begin implementation to accommodate
    // const and non-const variants.
    template <typename Ctr>
    static auto begin_impl(Ctr &c)
    {
        // The return type will be iterator/const_iterator,
        // depending on the constness of Ctr.
        using ret_it_t = ::std::conditional_t<::std::is_const_v<Ctr>, const_iterator, iterator>;

        ret_it_t retval(&c, 0);
        // Look for a non-empty poly.
        for (auto &p : c) {
            if (!p.second.empty()) {
                retval.m_local_it = p.second.begin();
                break;
            }
            ++retval.m_idx;
        }

        // NOTE: if all the poly are empty, m_idx is now
        // set to the size of the container
        // and the local iterator stays in its value-inited
        // state. That is, retval becomes the end iterator.
        return retval;
    }

public:
    const_iterator begin() const noexcept
    {
        return tps::begin_impl(m_container);
    }
    const_iterator end() const noexcept
    {
        // NOTE: end iterators contain a value-inited
        // local iterator.
        return const_iterator(&m_container, m_container.size());
    }
    const_iterator cbegin() const noexcept
    {
        return begin();
    }
    const_iterator cend() const noexcept
    {
        return end();
    }
    iterator begin() noexcept
    {
        return tps::begin_impl(m_container);
    }
    iterator end() noexcept
    {
        return iterator(&m_container, m_container.size());
    }

private:
    symbol_set m_symbol_set;
    container_t m_container;
};

} // namespace power_series

template <typename K, typename C>
using tps = power_series::tps<K, C>;

} // namespace obake

#endif
