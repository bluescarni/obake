// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_SERIES_HPP
#define PIRANHA_SERIES_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/container/small_vector.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/abseil.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/detail/not_implemented.hpp>
#include <piranha/detail/priority_tag.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/detail/tcast.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/hash.hpp>
#include <piranha/key/key_is_compatible.hpp>
#include <piranha/key/key_is_zero.hpp>
#include <piranha/key/key_stream_insert.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/math/negate.hpp>
#include <piranha/math/pow.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>
#include <piranha/utils/type_name.hpp>

namespace piranha
{

template <typename T>
using is_key = ::std::conjunction<is_semi_regular<T>, is_hashable<::std::add_lvalue_reference_t<const T>>,
                                  is_equality_comparable<::std::add_lvalue_reference_t<const T>>,
                                  is_zero_testable_key<::std::add_lvalue_reference_t<const T>>,
                                  is_compatibility_testable_key<::std::add_lvalue_reference_t<const T>>,
                                  is_stream_insertable_key<::std::add_lvalue_reference_t<const T>>>;

template <typename T>
inline constexpr bool is_key_v = is_key<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Key = is_key_v<T>;

#endif

template <typename T>
using is_cf = ::std::conjunction<
    is_semi_regular<T>, is_zero_testable<::std::add_lvalue_reference_t<const T>>,
    is_stream_insertable<::std::add_lvalue_reference_t<const T>>,
    is_compound_addable<::std::add_lvalue_reference_t<T>, ::std::add_lvalue_reference_t<const T>>,
    is_compound_addable<::std::add_lvalue_reference_t<T>, ::std::add_rvalue_reference_t<T>>,
    is_compound_subtractable<::std::add_lvalue_reference_t<T>, ::std::add_lvalue_reference_t<const T>>,
    is_compound_subtractable<::std::add_lvalue_reference_t<T>, ::std::add_rvalue_reference_t<T>>,
    is_negatable<::std::add_lvalue_reference_t<T>>>;

template <typename T>
inline constexpr bool is_cf_v = is_cf<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Cf = is_cf_v<T>;

#endif

namespace detail
{

// A small hashing wrapper for keys. It accomplishes two tasks:
// - force the evaluation of a key through const reference,
//   so that, in the Key requirements, we can request hashability
//   through const lvalue ref;
// - provide additional mixing.
struct key_hasher {
    static ::std::size_t hash_mixer(const ::std::size_t &h) noexcept
    {
        return ::absl::Hash<::std::size_t>{}(h);
    }
    template <typename K>
    ::std::size_t operator()(const K &k) const noexcept(noexcept(::piranha::hash(k)))
    {
        return key_hasher::hash_mixer(::piranha::hash(k));
    }
};

// Wrapper to force key comparison via const lvalue refs.
struct key_comparer {
    template <typename K>
    constexpr bool operator()(const K &k1, const K &k2) const noexcept(noexcept(k1 == k2))
    {
        return k1 == k2;
    }
};

// Helper for inserting a term into a series table.
template <bool CheckZero, bool CheckTableSize, typename S, typename Table, typename T, typename U>
inline void series_add_term_table(S &s, Table &t, T &&key, U &&cf)
{
    // Determine the coefficient type.
    // NOTE: we always assume that T and U are the exact
    // series key/cf types, after removal of ref and cv qualifiers.
    using cf_type = remove_cvref_t<U>;

    if constexpr (CheckTableSize) {
        if (piranha_unlikely(t.size() == s.get_max_table_size())) {
            // The table size is already the maximum allowed, don't
            // attempt the insertion.
            piranha_throw(::std::overflow_error, "Cannot attempt the insertion of a new term into a series: the "
                                                 "destination table already contains the maximum number of terms ("
                                                     + detail::to_string(s.get_max_table_size()) + ")");
        }
    }

    // Attempt the insertion.
    const auto res = t.try_emplace(detail::tcast(::std::forward<T>(key)), detail::tcast(::std::forward<U>(cf)));

    if (!res.second) {
        // Insertion did not take place because a term with
        // the same key exists already. Add the input coefficient
        // to the existing one.
        // NOTE: after this line, we have modified a coefficient
        // in the series, and the series might now be in an invalid
        // state. From now on, we have to pay attention to exception
        // safety.
        res.first->second += detail::tcast(::std::forward<U>(cf));
        // Now check that the updated coefficient is not zero.
        if constexpr (CheckZero) {
            try {
                if (piranha_unlikely(::piranha::is_zero(static_cast<const cf_type &>(res.first->second)))) {
                    t.erase(res.first);
                }
            } catch (...) {
                // NOTE: if is_zero() throws, we may have the table
                // in an invalid state (a coefficient may be zero).
                // Clear it before re-throwing.
                t.clear();
                throw;
            }
        }
    }
}

// TODO:
// - negative insertion,
// - exception safety in case of negative insertion,
// - double check exception safety with the old code.
// Helper for inserting a term into a series.
template <bool CheckZero, bool CheckCompatKey, bool CheckTableSize, typename S, typename T, typename U>
inline void series_add_term(S &s, T &&key, U &&cf)
{
    // Determine key/cf types.
    // NOTE: we always assume that T and U are the exact
    // series key/cf types, after removal of ref and cv qualifiers.
    using key_type = remove_cvref_t<T>;
    using cf_type = remove_cvref_t<U>;

    // Cache a couple of quantities.
    const auto &ss = s.m_symbol_set;
    const auto log2_size = s.m_log2_size;

    // Run checks early. We want to optimise the case
    // in which we are actually inserting something in the series.
    if constexpr (CheckCompatKey) {
        if (piranha_unlikely(!::piranha::key_is_compatible(static_cast<const key_type &>(key), ss))) {
            // The key is not compatible with the symbol set.
            if constexpr (is_stream_insertable_v<const key_type &>) {
                // A slightly better error message if we can
                // produce a string representation of the key.
                ::std::ostringstream oss;
                static_cast<::std::ostream &>(oss) << static_cast<const key_type &>(key);
                piranha_throw(::std::invalid_argument,
                              "Cannot add a new term to a series: the term's key, \"" + oss.str()
                                  + "\", is not compatible with the series' symbol set, " + detail::to_string(ss));
            } else {
                piranha_throw(::std::invalid_argument, "Cannot add a new term to a series: the term's key is not "
                                                       "compatible with the series' symbol set, "
                                                           + detail::to_string(ss));
            }
        }
    }

    if constexpr (CheckZero) {
        if (piranha_unlikely(::piranha::key_is_zero(static_cast<const key_type &>(key), ss)
                             || ::piranha::is_zero(static_cast<const cf_type &>(cf)))) {
            // If either key or coefficient are zero, no
            // insertion will take place.
            return;
        }
    }

    if (log2_size == 0u) {
        // NOTE: forcibly set CheckTableSize to false (for a single
        // table, the size limit is always the full range of size_type).
        detail::series_add_term_table<CheckZero, false>(s, s.s_table[0], ::std::forward<T>(key), ::std::forward<U>(cf));
    } else {
        // Compute the hash of the key via piranha::hash().
        const auto k_hash = ::piranha::hash(static_cast<const key_type &>(key));

        // Determine the destination table.
        const auto table_idx = static_cast<decltype(s.s_table.size())>(k_hash & (log2_size - 1u));

        // Proceed to the insertion.
        detail::series_add_term_table<CheckZero, CheckTableSize>(s, s.s_table[table_idx], ::std::forward<T>(key),
                                                                 ::std::forward<U>(cf));
    }
}

} // namespace detail

// TODO: document that moved-from series are destructible and assignable.
// TODO: test singular iterators.
// TODO: check construction of const iterators from murable ones.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <Key K, Cf C, typename Tag>
#else
template <typename K, typename C, typename Tag,
          typename = ::std::enable_if_t<::std::conjunction_v<is_key<K>, is_cf<C>>>>
#endif
class series
{
    // Make friends with the term insertion helpers.
    template <bool, bool, bool, typename S, typename T, typename U>
    friend void detail::series_add_term(S &, T &&, U &&);
    template <bool, bool, typename S, typename M, typename T, typename U>
    friend void detail::series_add_term_table(S &, M &, T &&, U &&);

    // Define the table type, and the type holding the set of tables (i.e., the segmented table).
    using table_type = ::absl::flat_hash_map<K, C, detail::key_hasher, detail::key_comparer>;
    using s_table_type = ::boost::container::small_vector<table_type, 1>;

    // Shortcut for the s_table size type.
    using s_size_t = typename s_table_type::size_type;

    // The maximum value of the m_log2_size member. Fix
    // it to the number of bits - 1 so that it's always
    // safe to bit shift a value of type s_size_t by
    // this amount.
    static constexpr s_size_t max_log2_size = static_cast<s_size_t>(detail::limits_digits<s_size_t> - 1);

public:
    using size_type = typename table_type::size_type;

    series() : s_table(1), m_log2_size(0) {}
    series(const series &) = default;
    series(series &&other) noexcept
        : s_table(::std::move(other.s_table)), m_log2_size(::std::move(other.m_log2_size)),
          m_symbol_set(::std::move(other.m_symbol_set))
    {
#if !defined(NDEBUG)
        // In debug mode, clear the other s_table
        // in order to flag that other was moved from.
        // This allows to run debug checks in the
        // destructor, if we know the series is in
        // a valid state.
        other.s_table.clear();
#endif
    }

    series &operator=(const series &) = default;
    series &operator=(series &&other) noexcept
    {
        // NOTE: assuming self-assignment is handled
        // correctly by the members.
        s_table = ::std::move(other.s_table);
        m_log2_size = ::std::move(other.m_log2_size);
        m_symbol_set = ::std::move(other.m_symbol_set);

#if !defined(NDEBUG)
        // NOTE: see above.
        other.s_table.clear();
#endif

        return *this;
    }

private:
    // Get the maximum size of the tables.
    // This will ensure that size_type can always
    // represent the total number of terms in the
    // series.
    size_type get_max_table_size() const
    {
        // NOTE: use a division rather than right shift,
        // so that we don't have to worry about shifting
        // too much. This will anyway be optimised into a
        // shift by the compiler.
        return static_cast<size_type>(::std::get<1>(detail::limits_minmax<size_type>) / (s_size_t(1) << m_log2_size));
    }

public:
    ~series()
    {
#if !defined(NDEBUG)
        // Don't run the checks if this
        // series was moved-from.
        if (!s_table.empty()) {
            // Make sure m_log2_size is within the limit.
            assert(m_log2_size <= max_log2_size);

            // Make sure the number of tables is consistent
            // with the log2 size.
            assert(s_table.size() == (s_size_t(1) << m_log2_size));

            // Make sure the size of each table does not exceed the limit.
            const auto mts = get_max_table_size();
            for (const auto &t : s_table) {
                assert(t.size() <= mts);
            }

            // Check all terms.
            for (const auto &p : *this) {
                // No zero terms.
                assert(!::piranha::key_is_zero(static_cast<const K &>(p.first), m_symbol_set)
                       && !::piranha::is_zero(static_cast<const C &>(p.second)));
                // No incompatible keys.
                assert(::piranha::key_is_compatible(static_cast<const K &>(p.first), m_symbol_set));
            }
        }
#endif
    }

    // Member function implementation of the swap primitive.
    void swap(series &other) noexcept
    {
        using ::std::swap;

        swap(s_table, other.s_table);
        swap(m_log2_size, other.m_log2_size);
        swap(m_symbol_set, other.m_symbol_set);
    }

    bool empty() const noexcept
    {
        return ::std::all_of(s_table.begin(), s_table.end(), [](const auto &table) { return table.empty(); });
    }

    size_type size() const noexcept
    {
        // NOTE: this will never overflow, as we enforce the constraint
        // that the size of each table is small enough to avoid overflows.
        return ::std::accumulate(s_table.begin(), s_table.end(), size_type(0),
                                 [](const auto &cur, const auto &table) { return cur + table.size(); });
    }

private:
    // A small helper to select the (const) iterator of s_table_type, depending on whether
    // T is const or not. Used in the iterator implementation below.
    template <typename T>
    using local_it_t = ::std::conditional_t<::std::is_const_v<T>, typename s_table_type::value_type::const_iterator,
                                            typename s_table_type::value_type::iterator>;

    // NOTE: this is mostly taken from:
    // https://www.boost.org/doc/libs/1_70_0/libs/iterator/doc/iterator_facade.html
    template <typename T>
    class iterator_impl
        : public ::boost::iterator_facade<iterator_impl<T>, T, ::boost::forward_traversal_tag, T &,
                                          // Fetch the difference type from the underlying local iterator.
                                          typename ::std::iterator_traits<local_it_t<T>>::difference_type>
    {
        // Make friends with other specialisations for interoperability
        // between const/non-const variants.
        template <typename U>
        friend class iterator_impl;
        // Make friends with boost's iterator machinery.
        friend class ::boost::iterator_core_access;
        // Make friends also with the series class, as sometimes
        // we need to poke the internals of the iterator from there.
        friend class series;

        // Select the type of the pointer to the s_table with the
        // correct constness.
        using s_table_ptr_t = ::std::conditional_t<::std::is_const_v<T>, const s_table_type *, s_table_type *>;

    public:
        // Defaul constructor.
        // NOTE: C++14 requires that all value-inited forward iterators
        // compare equal. This is guaranteed by this constructor, since
        // local_it_t is also a forward iterator which is default-inited.
        // https://en.cppreference.com/w/cpp/named_req/ForwardIterator
        iterator_impl() : s_table_ptr(nullptr), m_idx(0), m_local_it{} {}

        // Init with a pointer to the s_table and an index.
        // Used in the begin()/end() implementations.
        // NOTE: ensure m_local_it is default-inited, so it is in
        // a known state. This is also exploited in the implementation
        // of begin()/end().
        explicit iterator_impl(s_table_ptr_t s_table_ptr, s_size_t idx)
            : s_table_ptr(s_table_ptr), m_idx(idx), m_local_it{}
        {
        }

        // Default the copy/move ctors.
        iterator_impl(const iterator_impl &) = default;
        iterator_impl(iterator_impl &&) = default;

        // Implicit converting ctor from another specialisation. This is
        // used to construct a const iterator from a mutable one.
        template <typename U,
                  ::std::enable_if_t<
                      ::std::conjunction_v<
                          // NOTE: prevent competition with the
                          // copy constructor.
                          ::std::negation<::std::is_same<T, U>>,
                          ::std::is_constructible<s_table_ptr_t, const typename iterator_impl<U>::s_table_ptr_t &>,
                          ::std::is_constructible<local_it_t<T>, const local_it_t<U> &>>,
                      int> = 0>
        iterator_impl(const iterator_impl<U> &other)
            : s_table_ptr(other.s_table_ptr), m_idx(other.m_idx), m_local_it(other.m_local_it)
        {
        }

        // Specialise the swap primitive.
        friend void swap(iterator_impl &it1, iterator_impl &it2) noexcept
        {
            using ::std::swap;

            swap(it1.s_table_ptr, it2.s_table_ptr);
            swap(it1.m_idx, it2.m_idx);
            // NOTE: all iterators are required to be swappable,
            // so this must always be supported by the local iterator
            // type. If it throws (unlikely), the program will
            // terminate as we are marking this function as noexcept.
            swap(it1.m_local_it, it2.m_local_it);
        }

    private:
        void increment()
        {
            // Must be pointing to something.
            assert(s_table_ptr != nullptr);
            // Cannot be already at the end of the s_table.
            assert(m_idx < s_table_ptr->size());
            // The current table cannot be empty.
            assert(!s_table_ptr[m_idx].empty());
            // The local iterator cannot be pointing
            // at the end of the current table.
            assert(m_local_it != (*s_table_ptr)[m_idx].end());

            // Move to the next item in the current table.
            auto &st = *s_table_ptr;
            ++m_local_it;

            // NOTE: we expect to have many more elements per
            // table than the total number of tables, hence
            // moving to the next table should be a relatively
            // rare occurrence.
            if (piranha_unlikely(m_local_it == st[m_idx].end())) {
                // We reached the end of the current table.
                // Keep bumping m_idx until we either
                // arrive in a non-empty table, or we reach
                // the end of the s_table.
                const auto st_size = st.size();
                while (true) {
                    ++m_idx;
                    if (m_idx == st_size) {
                        // End of the s_table, reset m_local_it to
                        // value-inited and exit.
                        // NOTE: this is important, because this is now an end()
                        // iterator and end() iterators contain a value-inited
                        // local iterator.
                        m_local_it = local_it_t<T>{};
                        break;
                    } else if (!st[m_idx].empty()) {
                        // The next non-empty table was found.
                        // Set m_local_it to its beginning and exit.
                        m_local_it = st[m_idx].begin();
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
            // Singular iterators are signalled by a null s_table pointer,
            // zero index and a singular local iterator. Thus, using this
            // comparison operator with two singular iterators will always
            // yield true, as required.
            //
            // Non-singular iterators must refer to the same s_table.
            // If they don't, we have UB (assertion failure in debug mode).
            assert(s_table_ptr == other.s_table_ptr);

            // NOTE: this is fine when comparing end() with itself,
            // as m_local_it as a unique representation for end
            // iterators.
            return (m_idx == other.m_idx && m_local_it == other.m_local_it);
        }

        T &dereference() const
        {
            // Must point to something.
            assert(s_table_ptr);
            // Must not be the end iterator.
            assert(m_idx < s_table_ptr->size());
            // The current table must not be empty.
            assert(!(*s_table_ptr)[m_idx].empty());
            // The local iterator must not point to the
            // end of the current table.
            assert(m_local_it != (*s_table_ptr)[m_idx].end());

            return *m_local_it;
        }

    private:
        s_table_ptr_t s_table_ptr;
        s_size_t m_idx;
        local_it_t<T> m_local_it;
    };

public:
    using iterator = iterator_impl<typename table_type::value_type>;
    using const_iterator = iterator_impl<const typename table_type::value_type>;

private:
    // Abstract out the begin implementation to accommodate
    // const and non-const variants.
    template <typename It, typename STable>
    static It begin_impl(STable &c)
    {
        It retval(&c, 0);
        // Look for a non-empty table.
        for (auto &table : c) {
            if (!table.empty()) {
                retval.m_local_it = table.begin();
                break;
            }
            ++retval.m_idx;
        }
        // NOTE: if all the tables are empty, or the s_table
        // is empty, m_idx is now set to the size of s_table
        // and the local iterator stays in its value-inited
        // state. That is, retval becomes the end iterator.
        return retval;
    }

public:
    const_iterator begin() const noexcept
    {
        return series::begin_impl<const_iterator>(s_table);
    }
    const_iterator end() const noexcept
    {
        // NOTE: end iterators contain a value-inited
        // local iterator.
        return const_iterator(&s_table, s_table.size());
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
        return series::begin_impl<iterator>(s_table);
    }
    iterator end() noexcept
    {
        return iterator(&s_table, s_table.size());
    }

    const symbol_set &get_symbol_set() const
    {
        return m_symbol_set;
    }
    void set_symbol_set(const symbol_set &s)
    {
        if (piranha_unlikely(!this->empty())) {
            piranha_throw(::std::invalid_argument,
                          "A symbol set can be set only in an empty series, but this series has "
                              + detail::to_string(this->size()) + " terms");
        }

        m_symbol_set = s;
    }

#if defined(PIRANHA_HAVE_CONCEPTS)
    template <SameCvr<K> T, SameCvr<C> U>
#else
    template <typename T, typename U,
              ::std::enable_if_t<::std::conjunction_v<is_same_cvr<T, K>, is_same_cvr<U, C>>, int> = 0>
#endif
    void add_term(T &&key, U &&cf)
    {
        // NOTE: all checks enabled.
        detail::series_add_term<true, true, true>(*this, ::std::forward<T>(key), ::std::forward<U>(cf));
    }

private:
    s_table_type s_table;
    s_size_t m_log2_size;
    symbol_set m_symbol_set;
};

// Free function implementation of the swapping primitive.
template <typename K, typename C, typename Tag>
inline void swap(series<K, C, Tag> &s1, series<K, C, Tag> &s2) noexcept
{
    s1.swap(s2);
}

namespace detail
{

template <typename T>
struct is_series_impl : ::std::false_type {
};

template <typename K, typename C, typename Tag>
struct is_series_impl<series<K, C, Tag>> : ::std::true_type {
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

template <typename K, typename C, typename Tag>
struct series_cf_t_impl<series<K, C, Tag>> {
    using type = C;
};

} // namespace detail

template <typename T>
using series_cf_t = typename detail::series_cf_t_impl<T>::type;

namespace detail
{

template <typename>
struct series_key_t_impl {
};

template <typename K, typename C, typename Tag>
struct series_key_t_impl<series<K, C, Tag>> {
    using type = K;
};

} // namespace detail

template <typename T>
using series_key_t = typename detail::series_key_t_impl<T>::type;

namespace detail
{

template <typename>
struct series_tag_t_impl {
};

template <typename K, typename C, typename Tag>
struct series_tag_t_impl<series<K, C, Tag>> {
    using type = Tag;
};

} // namespace detail

template <typename T>
using series_tag_t = typename detail::series_tag_t_impl<T>::type;

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

namespace customisation
{

// External customisation point for piranha::series_stream_insert().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto series_stream_insert = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto series_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<2>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::series_stream_insert<T &&>)(os, ::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto series_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION(series_stream_insert(os, ::std::forward<T>(x)));

// Lowest priority: the default implementation for series.
template <typename T, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
inline auto series_stream_insert_impl(::std::ostream &os, T &&s_, priority_tag<0>)
{
    using series_t = remove_cvref_t<T>;
    using cf_type = series_cf_t<series_t>;
    using key_type = series_key_t<series_t>;

    const auto &s = static_cast<const series_t &>(s_);
    const auto &ss = s.get_symbol_set();

    // Print the header.
    os << "Key type        : " << ::piranha::type_name<key_type>() << '\n';
    os << "Coefficient type: " << ::piranha::type_name<cf_type>() << '\n';
    os << "Tag type        : " << ::piranha::type_name<series_tag_t<series_t>>() << '\n';
    os << "Symbol set      : " << detail::to_string(s.get_symbol_set()) << '\n';
    os << "Number of terms : " << s.size() << '\n';

    if (s.empty()) {
        // Special-case an empty series.
        os << '0';
        return;
    }

    // Hard-code the limit for now.
    constexpr auto limit = 50ul;

    decltype(s.size()) count = 0;
    auto it = s.begin();
    const auto end = s.end();
    ::std::ostringstream oss;
    ::std::string ret;

    for (; it != end;) {
        if (limit && count == limit) {
            break;
        }

        // Get the string representations of coefficient and key.
        oss.str("");
        static_cast<::std::ostream &>(oss) << static_cast<const cf_type &>(it->second);
        auto str_cf = oss.str();

        oss.str("");
        ::piranha::key_stream_insert(static_cast<::std::ostream &>(oss), static_cast<const key_type &>(it->first), ss);
        const auto str_key = oss.str();

        if (str_cf == "1" && str_key != "1") {
            // Suppress the coefficient if it is "1"
            // and the key is not "1".
            str_cf.clear();
        } else if (str_cf == "-1" && str_key != "1") {
            // Turn the coefficient into a minus sign
            // if it is -1 and the key is not "1".
            str_cf = '-';
        }

        // Append the coefficient.
        ret += str_cf;
        if (!str_cf.empty() && str_cf != "-" && str_key != "1") {
            // If the abs(coefficient) is not unitary and
            // the key is not "1", then we need the multiplication sign.
            ret += '*';
        }

        // Append the key, if it is not unitary.
        if (str_key != "1") {
            ret += str_key;
        }

        // Increase the counters.
        ++it;
        if (it != end) {
            // Prepare the plus for the next term
            // if we are not at the end.
            ret += '+';
        }
        ++count;
    }

    // If we reached the limit without printing all terms in the series, print the ellipsis.
    if (limit && count == limit && it != end) {
        ret += "...";
    }

    // Transform "+-" into "-".
    ::std::string::size_type index = 0;
    while (true) {
        index = ret.find("+-", index);
        if (index == ::std::string::npos) {
            break;
        }
        ret.replace(index++, 2, 1, '-');
    }

    os << ret;
}

} // namespace detail

inline constexpr auto series_stream_insert = [](::std::ostream & os, auto &&s) PIRANHA_SS_FORWARD_LAMBDA(
    detail::series_stream_insert_impl(os, ::std::forward<decltype(s)>(s), detail::priority_tag<2>{}));

// NOTE: constrain the operator so that it is enabled
// only if s is a series. This way, we avoid it to be too
// greedy.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <CvrSeries S>
#else
template <typename S, ::std::enable_if_t<is_cvr_series_v<S>, int> = 0>
#endif
constexpr auto operator<<(::std::ostream &os, S &&s)
    PIRANHA_SS_FORWARD_FUNCTION((void(::piranha::series_stream_insert(os, ::std::forward<S>(s))), os));

} // namespace piranha

#endif
