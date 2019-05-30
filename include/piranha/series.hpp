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
#include <piranha/detail/ignore.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/detail/not_implemented.hpp>
#include <piranha/detail/priority_tag.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/detail/tcast.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/hash.hpp>
#include <piranha/key/key_is_compatible.hpp>
#include <piranha/key/key_is_one.hpp>
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

// NOTE: runtime requirements:
// - constructor from symbol_set generates unitary key
//   compatible with the input symbol set.
template <typename T>
using is_key = ::std::conjunction<is_semi_regular<T>, is_constructible<T, const symbol_set &>,
                                  is_hashable<::std::add_lvalue_reference_t<const T>>,
                                  is_equality_comparable<::std::add_lvalue_reference_t<const T>>,
                                  is_zero_testable_key<::std::add_lvalue_reference_t<const T>>,
                                  is_one_testable_key<::std::add_lvalue_reference_t<const T>>,
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

} // namespace detail

// Forward declaration.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <Key, Cf, typename>
#else
template <typename K, typename C, typename, typename = ::std::enable_if_t<::std::conjunction_v<is_key<K>, is_cf<C>>>>
#endif
class series;

namespace detail
{

template <typename T>
inline constexpr ::std::size_t series_rank_impl = 0;

template <typename K, typename C, typename Tag>
inline constexpr ::std::size_t series_rank_impl<series<K, C, Tag>> = []() {
    static_assert(series_rank_impl<C> < ::std::get<1>(limits_minmax<::std::size_t>), "Overflow error");
    return series_rank_impl<C> + 1u;
}();

} // namespace detail

template <typename T>
inline constexpr ::std::size_t series_rank = detail::series_rank_impl<T>;

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

namespace detail
{

// A bunch of scoped enums used to fine-tune at compile-time
// the behaviour of the term insertion helpers below.
// NOTE: use scoped enums instead of plain bools to avoid
// mixing up the order of the flags when invoking the helpers.
enum class sat_check_zero : bool { off, on };
enum class sat_check_compat_key : bool { off, on };
enum class sat_check_table_size : bool { off, on };
enum class sat_assume_unique : bool { off, on };

// Helper for inserting a term into a series table.
template <bool Sign, sat_check_zero CheckZero, sat_check_compat_key CheckCompatKey, sat_check_table_size CheckTableSize,
          sat_assume_unique AssumeUnique, typename S, typename Table, typename T, typename... Args>
inline void series_add_term_table(S &s, Table &t, T &&key, Args &&... args)
{
    // Determine the key/cf types.
    using key_type = series_key_t<::std::remove_reference_t<S>>;
    using cf_type = series_cf_t<::std::remove_reference_t<S>>;
    static_assert(::std::is_same_v<key_type, remove_cvref_t<T>>);

    // Cache a reference to the symbol set.
    const auto &ss = s.m_symbol_set;

    if constexpr (CheckTableSize == sat_check_table_size::on) {
        // Check the table size, if requested.
        if (piranha_unlikely(t.size() == s.get_max_table_size())) {
            // The table size is already the maximum allowed, don't
            // attempt the insertion.
            piranha_throw(::std::overflow_error, "Cannot attempt the insertion of a new term into a series: the "
                                                 "destination table already contains the maximum number of terms ("
                                                     + detail::to_string(s.get_max_table_size()) + ")");
        }
    }

    if constexpr (CheckCompatKey == sat_check_compat_key::on) {
        // Check key for compatibility, if requested.
        if (piranha_unlikely(!::piranha::key_is_compatible(static_cast<const key_type &>(key), ss))) {
            // The key is not compatible with the symbol set.
            if constexpr (is_stream_insertable_v<const key_type &>) {
                // A slightly better error message if we can
                // produce a string representation of the key.
                ::std::ostringstream oss;
                static_cast<::std::ostream &>(oss) << static_cast<const key_type &>(key);
                piranha_throw(::std::invalid_argument, "Cannot add a term to a series: the term's key, \"" + oss.str()
                                                           + "\", is not compatible with the series' symbol set, "
                                                           + detail::to_string(ss));
            } else {
                piranha_throw(::std::invalid_argument, "Cannot add a term to a series: the term's key is not "
                                                       "compatible with the series' symbol set, "
                                                           + detail::to_string(ss));
            }
        }
    } else {
        // Otherwise, assert that the key is compatible.
        // There are no situations so far in which we may
        // want to allow adding an incompatible key.
        assert(::piranha::key_is_compatible(static_cast<const key_type &>(key), ss));
    }

    // Attempt the insertion.
    const auto res = t.try_emplace(detail::tcast(::std::forward<T>(key)), ::std::forward<Args>(args)...);

    if constexpr (AssumeUnique == sat_assume_unique::on) {
        // Assert that we actually performed an insertion,
        // in case we are assuming the term is unique.
        assert(res.second);
    }

    try {
        if (AssumeUnique == sat_assume_unique::on || res.second) {
            // The insertion took place. Change
            // the sign of the newly-inserted term,
            // in case of negative insertion.
            if constexpr (!Sign) {
                ::piranha::negate(static_cast<cf_type &>(res.first->second));
            }
        } else {
            // The insertion did not take place because a term with
            // the same key exists already. Add/sub the input coefficient
            // to/from the existing one.

            // Determine if we are inserting a coefficient, or
            // a pack that can be used to construct a coefficient.
            constexpr auto args_is_cf = []() {
                if constexpr (sizeof...(args) == 1u) {
                    return ::std::is_same_v<cf_type, remove_cvref_t<Args>...>;
                } else {
                    return false;
                }
            }();

            if constexpr (Sign) {
                if constexpr (args_is_cf) {
                    // NOTE: if we are inserting a coefficient, use it directly.
                    res.first->second += detail::tcast(::std::forward<Args>(args)...);
                } else {
                    // Otherwise, construct a coefficient from the input pack
                    // and add that instead.
                    res.first->second += cf_type(::std::forward<Args>(args)...);
                }
            } else {
                if constexpr (args_is_cf) {
                    res.first->second -= detail::tcast(::std::forward<Args>(args)...);
                } else {
                    res.first->second -= cf_type(::std::forward<Args>(args)...);
                }
            }
        }

        if constexpr (CheckZero == sat_check_zero::on) {
            // If requested, check whether the term we inserted
            // or modified is zero. If it is, erase it.
            if (piranha_unlikely(::piranha::key_is_zero(static_cast<const key_type &>(res.first->first), ss)
                                 || ::piranha::is_zero(static_cast<const cf_type &>(res.first->second)))) {
                t.erase(res.first);
            }
        }
    } catch (...) {
        // NOTE: if something threw, the table might now be in an
        // inconsistent state. Clear it out before rethrowing.
        t.clear();

        throw;
    }
}

// Helper for inserting a term into a series.
template <bool Sign, sat_check_zero CheckZero, sat_check_compat_key CheckCompatKey, sat_check_table_size CheckTableSize,
          sat_assume_unique AssumeUnique, typename S, typename T, typename... Args>
inline void series_add_term(S &s, T &&key, Args &&... args)
{
    // Determine the key type.
    using key_type = series_key_t<::std::remove_reference_t<S>>;
    static_assert(::std::is_same_v<key_type, remove_cvref_t<T>>);

    const auto log2_size = s.m_log2_size;
    if (log2_size == 0u) {
        // NOTE: forcibly set the table size check to off (for a single
        // table, the size limit is always the full range of size_type).
        detail::series_add_term_table<Sign, CheckZero, CheckCompatKey, sat_check_table_size::off, AssumeUnique>(
            s, s.m_s_table[0], ::std::forward<T>(key), ::std::forward<Args>(args)...);
    } else {
        // Compute the hash of the key via piranha::hash().
        const auto k_hash = ::piranha::hash(static_cast<const key_type &>(key));

        // Determine the destination table.
        const auto table_idx = static_cast<decltype(s.m_s_table.size())>(k_hash & (log2_size - 1u));

        // Proceed to the insertion.
        detail::series_add_term_table<Sign, CheckZero, CheckCompatKey, CheckTableSize, AssumeUnique>(
            s, s.m_s_table[table_idx], ::std::forward<T>(key), ::std::forward<Args>(args)...);
    }
}

// Machinery for series' generic constructor.
template <typename T, typename K, typename C, typename Tag>
constexpr int series_generic_ctor_algorithm_impl()
{
    using rT = remove_cvref_t<T>;
    using series_t = series<K, C, Tag>;

    if constexpr (::std::is_same_v<rT, series_t>) {
        // Avoid competition with the copy/move ctors.
        return 0;
    } else if constexpr (series_rank<rT> < series_rank<series_t>) {
        // Construction from lesser rank requires
        // to be able to construct C from T.
        return is_constructible_v<C, T> ? 1 : 0;
    } else if constexpr (series_rank<rT> == series_rank<series_t>) {
        if constexpr (::std::conjunction_v<::std::is_same<series_key_t<rT>, K>,
                                           ::std::is_same<series_tag_t<rT>, Tag>>) {
            // Construction from equal rank, different coefficient type. Requires
            // to be able to construct C from the coefficient type of T.
            // The construction argument will be a const reference or an rvalue
            // reference, depending on whether T is a mutable rvalue reference.
            using cf_conv_t
                = ::std::conditional_t<is_mutable_rvalue_reference_v<T>, series_cf_t<rT> &&, const series_cf_t<rT> &>;
            return is_constructible_v<C, cf_conv_t> ? 2 : 0;
        } else {
            return 0;
        }
    } else {
        // Construction from higher rank. Requires that
        // series_t can be constructed from the coefficient
        // type of T.
        using series_conv_t
            = ::std::conditional_t<is_mutable_rvalue_reference_v<T>, series_cf_t<rT> &&, const series_cf_t<rT> &>;
        return is_constructible_v<series_t, series_conv_t> ? 3 : 0;
    }
}

template <typename T, typename K, typename C, typename Tag>
inline constexpr int series_generic_ctor_algorithm = detail::series_generic_ctor_algorithm_impl<T, K, C, Tag>();

} // namespace detail

template <typename T, typename K, typename C, typename Tag>
using is_series_interoperable
    = ::std::integral_constant<bool, detail::series_generic_ctor_algorithm<T, K, C, Tag> != 0>;

template <typename T, typename K, typename C, typename Tag>
inline constexpr bool is_series_interoperable_v = is_series_interoperable<T, K, C, Tag>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename K, typename C, typename Tag>
PIRANHA_CONCEPT_DECL SeriesInteroperable = is_series_interoperable_v<T, K, C, Tag>;

#endif

// TODO: document that moved-from series are destructible and assignable.
// TODO: test singular iterators.
// TODO: check construction of const iterators from murable ones.
// TODO: test construction of series with int coefficient from
// series with double coefficient, to verify that coefficients
// that get truncated to zero are not inserted.
// TODO: test move semantics during insertion, generic construction.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <Key K, Cf C, typename Tag>
#else
template <typename K, typename C, typename Tag, typename>
#endif
class series
{
    // Make friends with the term insertion helpers.
    template <bool, detail::sat_check_zero, detail::sat_check_compat_key, detail::sat_check_table_size,
              detail::sat_assume_unique, typename S, typename T, typename... Args>
    friend void detail::series_add_term(S &, T &&, Args &&...);
    template <bool, detail::sat_check_zero, detail::sat_check_compat_key, detail::sat_check_table_size,
              detail::sat_assume_unique, typename S, typename Table, typename T, typename... Args>
    friend void detail::series_add_term_table(S &, Table &, T &&, Args &&...);

    // Make friends with all series types.
    template <typename, typename, typename
#if !defined(PIRANHA_HAVE_CONCEPTS)
              ,
              typename
#endif
              >
    friend class series;

    // Define the table type, and the type holding the set of tables (i.e., the segmented table).
    using table_type = ::absl::flat_hash_map<K, C, detail::key_hasher, detail::key_comparer>;
    using s_table_type = ::boost::container::small_vector<table_type, 1>;

    // Shortcut for the segmented table size type.
    using s_size_t = typename s_table_type::size_type;

    // The maximum value of the m_log2_size member. Fix
    // it to the number of bits - 1 so that it's always
    // safe to bit shift a value of type s_size_t by
    // this amount.
    static constexpr s_size_t max_log2_size = static_cast<s_size_t>(detail::limits_digits<s_size_t> - 1);

public:
    using size_type = typename table_type::size_type;

    series() : m_s_table(1), m_log2_size(0) {}
    series(const series &) = default;
    series(series &&other) noexcept
        : m_s_table(::std::move(other.m_s_table)), m_log2_size(::std::move(other.m_log2_size)),
          m_symbol_set(::std::move(other.m_symbol_set))
    {
#if !defined(NDEBUG)
        // In debug mode, clear the other segmented table
        // in order to flag that other was moved from.
        // This allows to run debug checks in the
        // destructor, if we know the series is in
        // a valid state.
        other.m_s_table.clear();
#endif
    }

#if defined(PIRANHA_HAVE_CONCEPTS)
    template <SeriesInteroperable<K, C, Tag> T>
#else
    template <typename T, ::std::enable_if_t<is_series_interoperable_v<T, K, C, Tag>, int> = 0>
#endif
    explicit series(T &&x) : series()
    {
        constexpr auto algo = detail::series_generic_ctor_algorithm<T, K, C, Tag>;

        // Small helper to clear x, if it is a nonconst
        // rvalue reference to a series. In some cases, we might
        // end up moving away individual coefficients from x,
        // which may leave x in an inconsistent state. With this
        // RAII struct, we'll ensure x is cleared out before
        // leaving this function (either as part of regular program
        // flow or in case of exception).
        struct x_clearer {
            x_clearer(T &&ref) : m_ref(::std::forward<T>(ref)) {}
            ~x_clearer()
            {
                if constexpr (is_mutable_rvalue_reference_v<T &&> && (series_rank<remove_cvref_t<T>>) > 0u) {
                    m_ref.clear();
                }
            }
            T &&m_ref;
        };

        if constexpr (algo == 1) {
            // Case 1: the series rank of T is less than the series
            // rank of this series type. Insert a term with unitary
            // key and coefficient constructed from x.

            // NOTE: disable key compat and table size checks: the key must be compatible
            // with the symbol set used for construction (key concept runtime requirement),
            // and we have only 1 table, so no size check needed. Also, the new term
            // will be unique by construction.
            detail::series_add_term_table<true, detail::sat_check_zero::on, detail::sat_check_compat_key::off,
                                          detail::sat_check_table_size::off, detail::sat_assume_unique::on>(
                *this, m_s_table[0], K(static_cast<const symbol_set &>(m_symbol_set)), ::std::forward<T>(x));
        } else if constexpr (algo == 2) {
            // Case 2: the series rank of T is equal to the series
            // rank of this series type, and the key and tag types
            // are the same (but the coefficient types are different,
            // otherwise we would be in a copy/move constructor scenario).
            // Insert all terms from x into this, converting the coefficients.

            // Init the clearer, as we'll be extracting
            // coefficients from x below.
            x_clearer xc(::std::forward<T>(x));

            // Reserve space in the new table.
            auto &tab = m_s_table[0];
            tab.reserve(x.size());

            // Copy/move over the symbol set.
            m_symbol_set = ::std::forward<T>(x).m_symbol_set;

            for (auto &p : x) {
                if constexpr (is_mutable_rvalue_reference_v<T &&>) {
                    // NOTE: like above, disable key compat check (we assume the other
                    // series contains only compatible keys) and table size check (only
                    // 1 table). We also know all the terms in the input
                    // series are unique. We keep the zero check because the conversion
                    // of the coefficient type of T to C might result in zero
                    // (e.g., converting from double to int).
                    detail::series_add_term_table<true, detail::sat_check_zero::on, detail::sat_check_compat_key::off,
                                                  detail::sat_check_table_size::off, detail::sat_assume_unique::on>(
                        *this, tab, p.first, ::std::move(p.second));
                } else {
                    detail::series_add_term_table<true, detail::sat_check_zero::on, detail::sat_check_compat_key::off,
                                                  detail::sat_check_table_size::off, detail::sat_assume_unique::on>(
                        *this, tab, p.first, static_cast<const series_cf_t<remove_cvref_t<T>> &>(p.second));
                }
            }
        } else if constexpr (algo == 3) {
            // Case 3: the series rank of T is higher than the series
            // rank of this series type. The construction is successful
            // only if x is a single coefficient series and this series
            // type is constructible from the coefficient type of T.

            if (x.is_single_cf()) {
                if (x.empty()) {
                    // If x is empty, just return (this is already empty).
                    assert(empty());
                    return;
                }

                // Init the clearer, as we will be moving the
                // only coefficient in x.
                x_clearer xc(::std::forward<T>(x));

                if constexpr (is_mutable_rvalue_reference_v<T &&>) {
                    *this = series(::std::move(x.begin()->second));
                } else {
                    *this = series(static_cast<const series_cf_t<remove_cvref_t<T>> &>(x.begin()->second));
                }
            } else {
                piranha_throw(::std::invalid_argument, "Cannot construct a series of type '"
                                                           + ::piranha::type_name<series>()
                                                           + "' from a series of higher rank of type '"
                                                           + ::piranha::type_name<remove_cvref_t<T>>()
                                                           + "' which does not consist of a single coefficient");
            }
        }
    }

    series &operator=(const series &) = default;
    series &operator=(series &&other) noexcept
    {
        // NOTE: assuming self-assignment is handled
        // correctly by the members.
        m_s_table = ::std::move(other.m_s_table);
        m_log2_size = ::std::move(other.m_log2_size);
        m_symbol_set = ::std::move(other.m_symbol_set);

#if !defined(NDEBUG)
        // NOTE: see above.
        other.m_s_table.clear();
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
        if (!m_s_table.empty()) {
            // Make sure m_log2_size is within the limit.
            assert(m_log2_size <= max_log2_size);

            // Make sure the number of tables is consistent
            // with the log2 size.
            assert(m_s_table.size() == (s_size_t(1) << m_log2_size));

            // Make sure the size of each table does not exceed the limit.
            const auto mts = get_max_table_size();
            for (const auto &t : m_s_table) {
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

            // Check that, in a segmented table, all terms are in the table they
            // belong to, according to the first-level hash.
            if (m_log2_size > 0u) {
                for (decltype(m_s_table.size()) i = 0u; i < m_s_table.size(); ++i) {
                    for (const auto &p : m_s_table[i]) {
                        assert((::piranha::hash(static_cast<const K &>(p.first)) & (m_log2_size - 1u)) == i);
                    }
                }
            }
        }
#endif
    }

    // Member function implementation of the swap primitive.
    void swap(series &other) noexcept
    {
        using ::std::swap;

        swap(m_s_table, other.m_s_table);
        swap(m_log2_size, other.m_log2_size);
        swap(m_symbol_set, other.m_symbol_set);
    }

    bool empty() const noexcept
    {
        return ::std::all_of(m_s_table.begin(), m_s_table.end(), [](const auto &table) { return table.empty(); });
    }

    size_type size() const noexcept
    {
        // NOTE: this will never overflow, as we enforce the constraint
        // that the size of each table is small enough to avoid overflows.
        return ::std::accumulate(m_s_table.begin(), m_s_table.end(), size_type(0),
                                 [](const auto &cur, const auto &table) { return cur + table.size(); });
    }

    bool is_single_cf() const
    {
        switch (size()) {
            case 0u:
                return true;
            case 1u:
                return ::piranha::key_is_one(cbegin()->first, m_symbol_set);
            default:
                return false;
        }
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

        // Select the type of the pointer to the segmented table
        // with the correct constness.
        using s_table_ptr_t = ::std::conditional_t<::std::is_const_v<T>, const s_table_type *, s_table_type *>;

    public:
        // Defaul constructor.
        // NOTE: C++14 requires that all value-inited forward iterators
        // compare equal. This is guaranteed by this constructor, since
        // local_it_t is also a forward iterator which is default-inited.
        // https://en.cppreference.com/w/cpp/named_req/ForwardIterator
        iterator_impl() : m_s_table_ptr(nullptr), m_idx(0), m_local_it{} {}

        // Init with a pointer to the segmented table and an index.
        // Used in the begin()/end() implementations.
        // NOTE: ensure m_local_it is default-inited, so it is in
        // a known state. This is also exploited in the implementation
        // of begin()/end().
        explicit iterator_impl(s_table_ptr_t s_table_ptr, s_size_t idx)
            : m_s_table_ptr(s_table_ptr), m_idx(idx), m_local_it{}
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
            : m_s_table_ptr(other.m_s_table_ptr), m_idx(other.m_idx), m_local_it(other.m_local_it)
        {
        }

        // Specialise the swap primitive.
        friend void swap(iterator_impl &it1, iterator_impl &it2) noexcept
        {
            using ::std::swap;

            swap(it1.m_s_table_ptr, it2.m_s_table_ptr);
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
            assert(m_s_table_ptr != nullptr);
            // Cannot be already at the end of the segmented table.
            assert(m_idx < m_s_table_ptr->size());
            // The current table cannot be empty.
            assert(!(*m_s_table_ptr)[m_idx].empty());
            // The local iterator cannot be pointing
            // at the end of the current table.
            assert(m_local_it != (*m_s_table_ptr)[m_idx].end());

            // Move to the next item in the current table.
            auto &st = *m_s_table_ptr;
            ++m_local_it;

            // NOTE: we expect to have many more elements per
            // table than the total number of tables, hence
            // moving to the next table should be a relatively
            // rare occurrence.
            if (piranha_unlikely(m_local_it == st[m_idx].end())) {
                // We reached the end of the current table.
                // Keep bumping m_idx until we either
                // arrive in a non-empty table, or we reach
                // the end of the segmented table.
                const auto st_size = st.size();
                while (true) {
                    ++m_idx;
                    if (m_idx == st_size) {
                        // End of the segmented table, reset m_local_it to
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
            // Singular iterators are signalled by a null segmented table pointer,
            // zero index and a singular local iterator. Thus, using this
            // comparison operator with two singular iterators will always
            // yield true, as required.
            //
            // Non-singular iterators must refer to the same segmented table.
            // If they don't, we have UB (assertion failure in debug mode).
            assert(m_s_table_ptr == other.m_s_table_ptr);

            // NOTE: this is fine when comparing end() with itself,
            // as m_local_it as a unique representation for end
            // iterators.
            return (m_idx == other.m_idx && m_local_it == other.m_local_it);
        }

        T &dereference() const
        {
            // Must point to something.
            assert(m_s_table_ptr);
            // Must not be the end iterator.
            assert(m_idx < m_s_table_ptr->size());
            // The current table must not be empty.
            assert(!(*m_s_table_ptr)[m_idx].empty());
            // The local iterator must not point to the
            // end of the current table.
            assert(m_local_it != (*m_s_table_ptr)[m_idx].end());

            return *m_local_it;
        }

    private:
        s_table_ptr_t m_s_table_ptr;
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
        // NOTE: if all the tables are empty, or the segmented table
        // is empty, m_idx is now set to the size of segmented table
        // and the local iterator stays in its value-inited
        // state. That is, retval becomes the end iterator.
        return retval;
    }

public:
    const_iterator begin() const noexcept
    {
        return series::begin_impl<const_iterator>(m_s_table);
    }
    const_iterator end() const noexcept
    {
        // NOTE: end iterators contain a value-inited
        // local iterator.
        return const_iterator(&m_s_table, m_s_table.size());
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
        return series::begin_impl<iterator>(m_s_table);
    }
    iterator end() noexcept
    {
        return iterator(&m_s_table, m_s_table.size());
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

    template <bool Sign = true,
#if defined(PIRANHA_HAVE_CONCEPTS)
              SameCvr<K> T, typename... Args>
    requires Constructible<C, Args...>
#else
              typename T, typename... Args,
              ::std::enable_if_t<::std::conjunction_v<is_same_cvr<T, K>, is_constructible<C, Args...>>, int> = 0>
#endif
        void add_term(T &&key, Args &&... args)
    {
        // NOTE: all checks enabled, don't assume uniqueness.
        detail::series_add_term<Sign, detail::sat_check_zero::on, detail::sat_check_compat_key::on,
                                detail::sat_check_table_size::on, detail::sat_assume_unique::off>(
            *this, ::std::forward<T>(key), ::std::forward<Args>(args)...);
    }

    // Set the number of segments (in log2 units).
    void set_nsegments(unsigned l)
    {
        if (piranha_unlikely(l > max_log2_size)) {
            piranha_throw(::std::invalid_argument, "Cannot set the number of segments to 2**" + detail::to_string(l)
                                                       + ", as this value exceeds the maximum allowed value (2**"
                                                       + detail::to_string(max_log2_size) + ")");
        }

        // NOTE: construct + move assign for exception safety.
        m_s_table = s_table_type(s_size_t(1) << l);
        m_log2_size = static_cast<s_size_t>(l);
    }

    // Clear the series.
    // This will remove all the terms and symbols.
    void clear() noexcept
    {
        for (auto &t : m_s_table) {
            t.clear();
        }
        m_symbol_set.clear();
    }

private:
    s_table_type m_s_table;
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
    os << "Rank            : " << series_rank<series_t> << '\n';
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

namespace customisation
{

// External customisation point for piranha::series_add().
template <typename T, typename U
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto series_add = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto series_add_impl(T &&x, U &&y, priority_tag<2>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::series_add<T &&, U &&>)(::std::forward<T>(x), ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto series_add_impl(T &&x, U &&y, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION(series_add(::std::forward<T>(x), ::std::forward<U>(y)));

template <typename T, typename U>
constexpr int series_add_strategy_impl()
{
    using rT [[maybe_unused]] = remove_cvref_t<T>;
    using rU [[maybe_unused]] = remove_cvref_t<U>;

    if constexpr (::std::conjunction_v<::std::negation<is_cvr_series<T>>, ::std::negation<is_cvr_series<U>>>) {
        // Neither T nor U are series, return 0. This will disable
        // the default series_add() implementation.
        return 0;
    } else if constexpr (!is_cvr_series_v<T>) {
        // T is a scalar, U is a series.
        // Determine the coefficient type of the return series type.
        using ret_cf_t = detected_t<detail::add_t, ::std::add_lvalue_reference_t<const rT>, const series_cf_t<rU> &>;
        if constexpr (is_cf_v<ret_cf_t>) {
            // The candidate coefficient type is valid. Establish
            // the series return type.
            using ret_t = series<series_key_t<rU>, ret_cf_t, series_tag_t<rU>>;
            return ::std::conjunction_v<is_constructible<ret_t, U>, is_constructible<ret_cf_t, T>> ? 1 : 0;
        } else {
            // The candidate return coefficient type
            // is not valid, or it does not
            // produce a coefficient type. Return 0.
            return 0;
        }
    } else if constexpr (!is_cvr_series_v<U>) {
        // Mirror of the above.
        using ret_cf_t = detected_t<detail::add_t, const series_cf_t<rT> &, ::std::add_lvalue_reference_t<const rU>>;
        if constexpr (is_cf_v<ret_cf_t>) {
            using ret_t = series<series_key_t<rT>, ret_cf_t, series_tag_t<rT>>;
            return ::std::conjunction_v<is_constructible<ret_t, T>, is_constructible<ret_cf_t, U>> ? 2 : 0;
        } else {
            return 0;
        }
    } else {
        // Both T and U are series.
    }
}

template <typename T, typename U>
inline constexpr int series_add_strategy = detail::series_add_strategy_impl<T, U>();

// Lowest priority: the default implementation for series.
template <typename T, typename U, ::std::enable_if_t<series_add_strategy<T &&, U &&> != 0, int> = 0>
constexpr auto series_add_impl(T &&x, U &&y, priority_tag<0>)
{
    using rT [[maybe_unused]] = remove_cvref_t<T>;
    using rU [[maybe_unused]] = remove_cvref_t<U>;

    constexpr auto strat = series_add_strategy<T &&, U &&>;

    if constexpr (strat == 1) {
        // T scalar, U series.
        using ret_t = series<series_key_t<rU>, detail::add_t<const rT &, const series_cf_t<rU> &>, series_tag_t<rU>>;

        ret_t retval(::std::forward<U>(y));
        // TODO fixme proper add_term.
        retval.add_term(series_key_t<rU>(retval.get_symbol_set()), ::std::forward<T>(x));

        return retval;
    } else if constexpr (strat == 2) {
        // T series, U scalar.
        using ret_t = series<series_key_t<rT>, detail::add_t<const series_cf_t<rT> &, const rU &>, series_tag_t<rT>>;

        ret_t retval(::std::forward<T>(x));
        retval.add_term(series_key_t<rT>(retval.get_symbol_set()), ::std::forward<U>(y));

        return retval;
    }
}

} // namespace detail

inline constexpr auto series_add = [](auto &&x, auto &&y) PIRANHA_SS_FORWARD_LAMBDA(
    detail::series_add_impl(::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<2>{}));

// Like with operator<<(), constrain so that the operator
// is enabled only if at least 1 operator is a series.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T> || CvrSeries<U>
#else
template <typename T, typename U, ::std::enable_if_t<::std::disjunction_v<is_cvr_series<T>, is_cvr_series<U>>, int> = 0>
#endif
constexpr auto operator+(T &&x, U &&y)
    PIRANHA_SS_FORWARD_FUNCTION(::piranha::series_add(::std::forward<T>(x), ::std::forward<U>(y)));

} // namespace piranha

#endif
