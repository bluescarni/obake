// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_SERIES_HPP
#define OBAKE_SERIES_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/container/container_fwd.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>

#include <mp++/integer.hpp>

#include <obake/byte_size.hpp>
#include <obake/cf/cf_stream_insert.hpp>
#include <obake/cf/cf_tex_stream_insert.hpp>
#include <obake/config.hpp>
#include <obake/detail/abseil.hpp>
#include <obake/detail/fcast.hpp>
#include <obake/detail/ignore.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/mppp_utils.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/type_c.hpp>
#include <obake/detail/visibility.hpp>
#include <obake/exceptions.hpp>
#include <obake/hash.hpp>
#include <obake/key/key_degree.hpp>
#include <obake/key/key_evaluate.hpp>
#include <obake/key/key_is_compatible.hpp>
#include <obake/key/key_is_one.hpp>
#include <obake/key/key_is_zero.hpp>
#include <obake/key/key_merge_symbols.hpp>
#include <obake/key/key_p_degree.hpp>
#include <obake/key/key_stream_insert.hpp>
#include <obake/key/key_tex_stream_insert.hpp>
#include <obake/key/key_trim.hpp>
#include <obake/key/key_trim_identify.hpp>
#include <obake/math/degree.hpp>
#include <obake/math/evaluate.hpp>
#include <obake/math/is_zero.hpp>
#include <obake/math/negate.hpp>
#include <obake/math/p_degree.hpp>
#include <obake/math/pow.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/math/safe_convert.hpp>
#include <obake/math/trim.hpp>
#include <obake/s11n.hpp>
#include <obake/symbols.hpp>
#include <obake/tex_stream_insert.hpp>
#include <obake/type_name.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

// NOTE: runtime requirements:
// - constructor from symbol_set generates unitary key
//   compatible with the input symbol set (this is used, for
//   instance, in series' generic constructor from a lower rank
//   series, and when preparing an output value for monomial_mul()).
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

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL Key = is_key_v<T>;

#endif

template <typename T>
using is_cf = ::std::conjunction<
    is_semi_regular<T>, is_zero_testable<::std::add_lvalue_reference_t<const T>>,
    is_stream_insertable_cf<::std::add_lvalue_reference_t<const T>>,
    is_compound_addable<::std::add_lvalue_reference_t<T>, ::std::add_lvalue_reference_t<const T>>,
    is_compound_addable<::std::add_lvalue_reference_t<T>, ::std::add_rvalue_reference_t<T>>,
    is_compound_subtractable<::std::add_lvalue_reference_t<T>, ::std::add_lvalue_reference_t<const T>>,
    is_compound_subtractable<::std::add_lvalue_reference_t<T>, ::std::add_rvalue_reference_t<T>>,
    is_negatable<::std::add_lvalue_reference_t<T>>>;

template <typename T>
inline constexpr bool is_cf_v = is_cf<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL Cf = is_cf_v<T>;

#endif

// Forward declaration.
#if defined(OBAKE_HAVE_CONCEPTS)
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
inline constexpr ::std::size_t series_rank_impl<series<K, C, Tag>> =
#if defined(_MSC_VER)
    series_rank_impl<C> + 1u
#else
    []() {
        static_assert(series_rank_impl<C> < limits_max<::std::size_t>, "Overflow error");
        return series_rank_impl<C> + 1u;
    }()
#endif
    ;

} // namespace detail

template <typename T>
inline constexpr ::std::size_t series_rank = detail::series_rank_impl<T>;

namespace detail
{

template <typename>
struct series_term_t_impl {
};

template <typename K, typename C, typename Tag>
struct series_term_t_impl<series<K, C, Tag>> {
    using type = remove_cvref_t<decltype(*::std::declval<series<K, C, Tag> &>().begin())>;
};

} // namespace detail

template <typename T>
using series_term_t = typename detail::series_term_t_impl<T>::type;

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

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL CvrSeries = is_cvr_series_v<T>;

#endif

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
    [[maybe_unused]] const auto &ss = s.get_symbol_set();

    if constexpr (CheckTableSize == sat_check_table_size::on) {
        // LCOV_EXCL_START
        // Check the table size, if requested.
        if (obake_unlikely(t.size() == s._get_max_table_size())) {
            // The table size is already the maximum allowed, don't
            // attempt the insertion.
            obake_throw(::std::overflow_error, "Cannot attempt the insertion of a new term into a series: the "
                                               "destination table already contains the maximum number of terms ("
                                                   + detail::to_string(s._get_max_table_size()) + ")");
        }
        // LCOV_EXCL_STOP
    }

    if constexpr (CheckCompatKey == sat_check_compat_key::on) {
        // Check key for compatibility, if requested.
        if (obake_unlikely(!::obake::key_is_compatible(::std::as_const(key), ss))) {
            // The key is not compatible with the symbol set.
            if constexpr (is_stream_insertable_v<const key_type &>) {
                // A slightly better error message if we can
                // produce a string representation of the key.
                ::std::ostringstream oss;
                static_cast<::std::ostream &>(oss) << ::std::as_const(key);
                obake_throw(::std::invalid_argument, "Cannot add a term to a series: the term's key, '" + oss.str()
                                                         + "', is not compatible with the series' symbol set, "
                                                         + detail::to_string(ss));
            } else {
                obake_throw(::std::invalid_argument, "Cannot add a term to a series: the term's key is not "
                                                     "compatible with the series' symbol set, "
                                                         + detail::to_string(ss));
            }
        }
    } else {
        // Otherwise, assert that the key is compatible.
        // There are no situations so far in which we may
        // want to allow adding an incompatible key.
        assert(::obake::key_is_compatible(::std::as_const(key), ss));
    }

    // Attempt the insertion.
    const auto res = t.try_emplace(detail::fcast(::std::forward<T>(key)), ::std::forward<Args>(args)...);

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
                ::obake::negate(res.first->second);
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
                    res.first->second += detail::fcast(::std::forward<Args>(args)...);
                } else {
                    // Otherwise, construct a coefficient from the input pack
                    // and add that instead.
                    res.first->second += cf_type(::std::forward<Args>(args)...);
                }
            } else {
                if constexpr (args_is_cf) {
                    res.first->second -= detail::fcast(::std::forward<Args>(args)...);
                } else {
                    res.first->second -= cf_type(::std::forward<Args>(args)...);
                }
            }
        }

        if constexpr (CheckZero == sat_check_zero::on) {
            // If requested, check whether the term we inserted
            // or modified is zero. If it is, erase it.
            if (obake_unlikely(::obake::key_is_zero(res.first->first, ss)
                               || ::obake::is_zero(::std::as_const(res.first->second)))) {
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

    auto &s_table = s._get_s_table();
    const auto s_table_size = s_table.size();
    assert(s_table_size > 0u);

    if (s_table_size == 1u) {
        // NOTE: forcibly set the table size check to off (for a single
        // table, the size limit is always the full range of size_type).
        detail::series_add_term_table<Sign, CheckZero, CheckCompatKey, sat_check_table_size::off, AssumeUnique>(
            s, s_table[0], ::std::forward<T>(key), ::std::forward<Args>(args)...);
    } else {
        // Compute the hash of the key via obake::hash().
        const auto k_hash = ::obake::hash(::std::as_const(key));

        // Determine the destination table.
        const auto table_idx = static_cast<decltype(s_table.size())>(k_hash & (s_table_size - 1u));

        // Proceed to the insertion.
        detail::series_add_term_table<Sign, CheckZero, CheckCompatKey, CheckTableSize, AssumeUnique>(
            s, s_table[table_idx], ::std::forward<T>(key), ::std::forward<Args>(args)...);
    }
}

// Machinery for series' generic constructor.
// NOTE: this can be improved to work also with series
// with same rank but different tag (same goes for the
// conversion operator).
template <typename T, typename K, typename C, typename Tag>
constexpr int series_generic_ctor_algorithm_impl()
{
    // NOTE: check first if series<K, C, Tag> is a well-formed
    // type (that is, K and C satisfy the key/cf requirements).
    // Like this, if this function is instantiated with bogus
    // types, it will return 0 rather than giving a hard error.
    if constexpr (is_detected_v<series, K, C, Tag>) {
        using series_t = series<K, C, Tag>;
        using rT = remove_cvref_t<T>;

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
                // reference, depending on whether T is a mutable rvalue reference or not.
                using cf_conv_t = ::std::conditional_t<is_mutable_rvalue_reference_v<T>, series_cf_t<rT> &&,
                                                       const series_cf_t<rT> &>;
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
    } else {
        return 0;
    }
}

template <typename T, typename K, typename C, typename Tag>
inline constexpr int series_generic_ctor_algorithm = detail::series_generic_ctor_algorithm_impl<T, K, C, Tag>();

} // namespace detail

template <typename T, typename K, typename C, typename Tag>
using is_series_constructible
    = ::std::integral_constant<bool, detail::series_generic_ctor_algorithm<T, K, C, Tag> != 0>;

template <typename T, typename K, typename C, typename Tag>
inline constexpr bool is_series_constructible_v = is_series_constructible<T, K, C, Tag>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T, typename K, typename C, typename Tag>
OBAKE_CONCEPT_DECL SeriesConstructible = is_series_constructible_v<T, K, C, Tag>;

#endif

template <typename T, typename C>
using is_series_convertible
    = ::std::conjunction<::std::integral_constant<bool, series_rank<T> == 0u>, ::std::is_object<T>,
                         is_constructible<T, int>, is_constructible<T, ::std::add_lvalue_reference_t<const C>>>;

template <typename T, typename C>
inline constexpr bool is_series_convertible_v = is_series_convertible<T, C>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T, typename C>
OBAKE_CONCEPT_DECL SeriesConvertible = is_series_convertible_v<T, C>;

#endif

namespace detail
{

// A small hashing wrapper for keys. It accomplishes two tasks:
// - force the evaluation of a key through const reference,
//   so that, in the Key requirements, we can request hashability
//   through const lvalue ref;
// - provide additional mixing.
struct series_key_hasher {
    static ::std::size_t hash_mixer(const ::std::size_t &h) noexcept
    {
        return ::absl::Hash<::std::size_t>{}(h);
    }
    template <typename K>
    ::std::size_t operator()(const K &k) const noexcept(noexcept(::obake::hash(k)))
    {
        return series_key_hasher::hash_mixer(::obake::hash(k));
    }
};

// Wrapper to force key comparison via const lvalue refs.
struct series_key_comparer {
    template <typename K>
    constexpr bool operator()(const K &k1, const K &k2) const noexcept(noexcept(k1 == k2))
    {
        return k1 == k2;
    }
};

// Small helper to clear() a nonconst
// rvalue reference to a series. This is used in various places
// where we might end up moving away individual coefficients from an input series,
// which may leave the series in an inconsistent state. With this
// RAII struct, we'll ensure the series is cleared out before
// leaving the scope (either as part of regular program
// flow or in case of exception).
// NOTE: this is different from the clear() that is called
// in debug mode during move operations: in that situation,
// we are guaranteeing that after the move the series
// is destructible and assignable, so the state of the series
// does not matter as long as we can revive it. This clearer,
// on the other hand, is not called during move operations,
// but only when it might make sense, for optimisation purposes,
// to move individual coefficients - hence the move semantics
// guarantee does not apply.
template <typename T>
struct series_rref_clearer {
    series_rref_clearer(T &&ref) : m_ref(::std::forward<T>(ref)) {}
    ~series_rref_clearer()
    {
        if constexpr (is_mutable_rvalue_reference_v<T &&>) {
            m_ref.clear();
        }
    }
    T &&m_ref;
};

// Helper to extend the keys of "from" with the symbol insertion map ins_map.
// The new series will be written to "to". The coefficient type of "to"
// may be different from the coefficient type of "from", in which case a coefficient
// conversion will take place. "to" is supposed to have to correct symbol set already,
// but, apart from that, it must be empty, and the number of segments and space
// reservation will be taken from "from".
template <typename To, typename From>
inline void series_sym_extender(To &to, From &&from, const symbol_idx_map<symbol_set> &ins_map)
{
    // NOTE: we assume that this helper is
    // invoked with a non-empty insertion map, and an empty
    // "to" series. "to" must have the correct symbol set.
    assert(!ins_map.empty());
    assert(to.empty());

    // Ensure that the key type of From
    // is symbol mergeable (via const lvalue ref).
    static_assert(is_symbols_mergeable_key_v<const series_key_t<remove_cvref_t<From>> &>);

    // We may end up moving coefficients from "from" in the conversion to "to".
    // Make sure we will clear "from" out properly.
    series_rref_clearer<From> from_c(::std::forward<From>(from));

    // Cache the original symbol set.
    const auto &orig_ss = from.get_symbol_set();

    // Set the number of segments, reserve space.
    const auto from_log2_size = from.get_s_size();
    to.set_n_segments(from_log2_size);
    to.reserve(::obake::safe_cast<decltype(to.size())>(from.size()));

    // Establish if we need to check for zero coefficients
    // when inserting. We don't if the coefficient types of to and from
    // coincide (i.e., no cf conversion takes place),
    // otherwise the conversion might generate zeroes.
    constexpr auto check_zero
        = static_cast<sat_check_zero>(::std::is_same_v<series_cf_t<To>, series_cf_t<remove_cvref_t<From>>>);

    // Merge the terms, distinguishing the segmented vs non-segmented case.
    if (from_log2_size) {
        for (auto &t : from._get_s_table()) {
            for (auto &term : t) {
                // NOTE: old clang does not like structured
                // bindings in the for loop.
                auto &k = term.first;
                auto &c = term.second;

                // Compute the merged key.
                auto merged_key = ::obake::key_merge_symbols(k, ins_map, orig_ss);

                // Insert the term. We need the following checks:
                // - zero check, in case the coefficient type changes,
                // - table size check, because even if we know the
                //   max table size was not exceeded in the original series,
                //   it might be now (as the merged key may end up in a different
                //   table).
                // NOTE: in the runtime requirements for key_merge_symbol(), we impose
                // that symbol merging does not affect is_zero(), compatibility and
                // uniqueness.
                if constexpr (is_mutable_rvalue_reference_v<From &&>) {
                    detail::series_add_term<true, check_zero, sat_check_compat_key::off, sat_check_table_size::on,
                                            sat_assume_unique::on>(to, ::std::move(merged_key), ::std::move(c));
                } else {
                    detail::series_add_term<true, check_zero, sat_check_compat_key::off, sat_check_table_size::on,
                                            sat_assume_unique::on>(to, ::std::move(merged_key), ::std::as_const(c));
                }
            }
        }
    } else {
        auto &to_table = to._get_s_table()[0];

        for (auto &t : from._get_s_table()[0]) {
            // NOTE: old clang does not like structured
            // bindings in the for loop.
            auto &k = t.first;
            auto &c = t.second;

            // Compute the merged key.
            auto merged_key = ::obake::key_merge_symbols(k, ins_map, orig_ss);

            // Insert the term: the only check we may need is check_zero, in case
            // the coefficient type changes. We know that the table size cannot be
            // exceeded as we are dealing with a single table.
            if constexpr (is_mutable_rvalue_reference_v<From &&>) {
                detail::series_add_term_table<true, check_zero, sat_check_compat_key::off, sat_check_table_size::off,
                                              sat_assume_unique::on>(to, to_table, ::std::move(merged_key),
                                                                     ::std::move(c));
            } else {
                detail::series_add_term_table<true, check_zero, sat_check_compat_key::off, sat_check_table_size::off,
                                              sat_assume_unique::on>(to, to_table, ::std::move(merged_key),
                                                                     ::std::as_const(c));
            }
        }
    }
}

} // namespace detail

// NOTE: document that moved-from series are destructible and assignable.
#if defined(OBAKE_HAVE_CONCEPTS)
template <Key K, Cf C, typename Tag>
#else
template <typename K, typename C, typename Tag, typename>
#endif
class series
{
    friend class ::boost::serialization::access;

public:
    // Define the table type, and the type holding the set of tables (i.e., the segmented table).
    using table_type = ::absl::flat_hash_map<K, C, detail::series_key_hasher, detail::series_key_comparer>;
    using s_table_type = ::boost::container::small_vector<table_type, 1>;

    // Shortcut for the segmented table size type.
    using s_size_type = typename s_table_type::size_type;

private:
    // The maximum value of the m_log2_size member. Fix
    // it to the number of bits - 1 so that it's always
    // safe to bit shift a value of type s_size_type by
    // this amount.
    static constexpr unsigned max_log2_size = static_cast<unsigned>(detail::limits_digits<s_size_type> - 1);

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
    // NOTE: the generic construction algorithm must be well-documented,
    // as we rely on its behaviour in a variety of places (e.g., when constructing
    // series from scalars).
#if defined(OBAKE_HAVE_CONCEPTS)
    template <SeriesConstructible<K, C, Tag> T>
#else
    template <typename T, ::std::enable_if_t<is_series_constructible_v<T &&, K, C, Tag>, int> = 0>
#endif
    explicit series(T &&x) : series()
    {
        constexpr int algo = detail::series_generic_ctor_algorithm<T &&, K, C, Tag>;
        static_assert(algo > 0 && algo <= 3);

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
                *this, m_s_table[0], K(::std::as_const(m_symbol_set)), ::std::forward<T>(x));
        } else if constexpr (algo == 2) {
            // Case 2: the series rank of T is equal to the series
            // rank of this series type, and the key and tag types
            // are the same (but the coefficient types are different,
            // otherwise we would be in a copy/move constructor scenario).
            // Insert all terms from x into this, converting the coefficients.
            static_assert(!::std::is_same_v<series_cf_t<remove_cvref_t<T>>, C>);

            // Init a rref clearer, as we may be extracting
            // coefficients from x below.
            detail::series_rref_clearer<T> xc(::std::forward<T>(x));

            // Copy over the symbol set.
            m_symbol_set = x.get_symbol_set();

            // Set the number of segments.
            const auto x_log2_size = x.get_s_size();
            set_n_segments(x_log2_size);

            // Insert the terms from x. Although the coefficients
            // change, the keys remain identical, so we can do the insertion
            // table by table, relying on the fact that the new keys
            // will hash to the same table indices as the original ones.
            // NOTE: this could be parallelised, if needed.
            for (s_size_type i = 0; i < (s_size_type(1) << x_log2_size); ++i) {
                // Extract references to the tables in x and this.
                auto &xt = x._get_s_table()[i];
                auto &tab = m_s_table[i];

                // Reserve space in the current table.
                tab.reserve(::obake::safe_cast<decltype(tab.size())>(xt.size()));

                for (auto &t : xt) {
                    // NOTE: old clang does not like structured
                    // bindings in the for loop.
                    auto &k = t.first;
                    auto &c = t.second;

                    // NOTE: like above, disable key compat check (we assume the other
                    // series contains only compatible keys) and table size check (we know
                    // the original tables do not exceed the max size).
                    // We also know all the terms in the input
                    // series are unique. We keep the zero check because the conversion
                    // of the coefficient type of T to C might result in zero
                    // (e.g., converting from double to int).
                    if constexpr (is_mutable_rvalue_reference_v<T &&>) {
                        detail::series_add_term_table<true, detail::sat_check_zero::on,
                                                      detail::sat_check_compat_key::off,
                                                      detail::sat_check_table_size::off, detail::sat_assume_unique::on>(
                            *this, tab, k, ::std::move(c));
                    } else {
                        detail::series_add_term_table<true, detail::sat_check_zero::on,
                                                      detail::sat_check_compat_key::off,
                                                      detail::sat_check_table_size::off, detail::sat_assume_unique::on>(
                            *this, tab, k, ::std::as_const(c));
                    }
                }
            }
        } else {
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

                // Init the clearer, as we might be moving the
                // only coefficient in x.
                detail::series_rref_clearer<T> xc(::std::forward<T>(x));

                if constexpr (is_mutable_rvalue_reference_v<T &&>) {
                    *this = series(::std::move(x.begin()->second));
                } else {
                    *this = series(::std::as_const(x.begin()->second));
                }
            } else {
                obake_throw(::std::invalid_argument,
                            "Cannot construct a series of type '" + ::obake::type_name<series>()
                                + "' from a series of higher rank of type '" + ::obake::type_name<remove_cvref_t<T>>()
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
#if defined(OBAKE_HAVE_CONCEPTS)
    template <SeriesConstructible<K, C, Tag> T>
#else
    template <typename T, ::std::enable_if_t<is_series_constructible_v<T &&, K, C, Tag>, int> = 0>
#endif
    series &operator=(T &&x)
    {
        return *this = series(::std::forward<T>(x));
    }

#if defined(OBAKE_HAVE_CONCEPTS)
    template <SeriesConvertible<C> T>
#else
    template <typename T, ::std::enable_if_t<is_series_convertible_v<T, C>, int> = 0>
#endif
    explicit operator T() const
    {
        switch (size()) {
            case 0u:
                return T(0);
            case 1u:
                return T(cbegin()->second);
            default:
                obake_throw(::std::invalid_argument,
                            "Cannot convert a series of type '" + ::obake::type_name<series>()
                                + "' to on object of type '" + ::obake::type_name<T>()
                                + "', because the series does not consist of a single coefficient");
        }
    }

    // NOTE: for segmented series, we could actually clear()
    // the tables in parallel so that the most expensive part
    // of the destruction process is faster.
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
            assert(m_s_table.size() == (s_size_type(1) << m_log2_size));

            // Make sure the size of each table does not exceed the limit.
            const auto mts = _get_max_table_size();
            for (const auto &t : m_s_table) {
                assert(t.size() <= mts);
            }

            // Check all terms.
            for (const auto &t : *this) {
                // NOTE: old clang does not like structured
                // bindings in the for loop.
                const auto &k = t.first;
                const auto &c = t.second;

                // No zero terms.
                assert(!::obake::key_is_zero(k, m_symbol_set) && !::obake::is_zero(c));
                // No incompatible keys.
                assert(::obake::key_is_compatible(k, m_symbol_set));
            }

            // Check that, in a segmented table, all terms are in the table they
            // belong to, according to the first-level hash.
            if (m_log2_size > 0u) {
                const auto s_table_size = m_s_table.size();
                for (s_size_type i = 0; i < s_table_size; ++i) {
                    for (const auto &p : m_s_table[i]) {
                        assert((::obake::hash(p.first) & (s_table_size - 1u)) == i);
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

    // Get the maximum size of the tables.
    // This will ensure that size_type can always
    // represent the total number of terms in the
    // series.
    size_type _get_max_table_size() const
    {
        // NOTE: use a division rather than right shift,
        // so that we don't have to worry about shifting
        // too much. This will anyway be optimised into a
        // shift by the compiler.
        return static_cast<size_type>(detail::limits_max<size_type> / (s_size_type(1) << m_log2_size));
    }

    unsigned get_s_size() const
    {
        return m_log2_size;
    }
    // NOTE: the guarantee that we can
    // always shift a s_size_type by this
    // value is important and needs to be
    // documents.
    static unsigned get_max_s_size()
    {
        return max_log2_size;
    }

    bool is_single_cf() const
    {
        switch (size()) {
            case 0u:
                return true;
            case 1u:
                return ::obake::key_is_one(cbegin()->first, m_symbol_set);
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
        explicit iterator_impl(s_table_ptr_t s_table_ptr, s_size_type idx)
            : m_s_table_ptr(s_table_ptr), m_idx(idx), m_local_it{}
        {
        }

        // Init with a pointer to the segmented table, an index and a local iterator.
        // Used in the find() implementations.
        explicit iterator_impl(s_table_ptr_t s_table_ptr, s_size_type idx, local_it_t<T> local_it)
            : m_s_table_ptr(s_table_ptr), m_idx(idx), m_local_it(local_it)
        {
            // NOTE: make sure this is never inited with the end
            // iterator of a table, as this is not the intended
            // use case.
            assert(local_it != (*s_table_ptr)[idx].end());
        }

        // Default the copy/move ctors/assignment operators.
        iterator_impl(const iterator_impl &) = default;
        iterator_impl(iterator_impl &&) = default;
        iterator_impl &operator=(const iterator_impl &) = default;
        iterator_impl &operator=(iterator_impl &&) = default;

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
            if (obake_unlikely(m_local_it == st[m_idx].end())) {
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
        s_size_type m_idx;
        local_it_t<T> m_local_it;
    };

public:
    using iterator = iterator_impl<typename table_type::value_type>;
    using const_iterator = iterator_impl<const typename table_type::value_type>;

private:
    // Abstract out the begin implementation to accommodate
    // const and non-const variants.
    template <typename STable>
    static auto begin_impl(STable &c)
    {
        // The return type will be iterator/const_iterator,
        // depending on the constness of STable.
        using ret_it_t = ::std::conditional_t<::std::is_const_v<STable>, const_iterator, iterator>;

        ret_it_t retval(&c, 0);
        // Look for a non-empty table.
        for (auto &table : c) {
            if (!table.empty()) {
                retval.m_local_it = table.begin();
                break;
            }
            ++retval.m_idx;
        }

        // NOTE: if all the tables are empty, m_idx is now
        // set to the size of segmented table
        // and the local iterator stays in its value-inited
        // state. That is, retval becomes the end iterator.
        return retval;
    }

public:
    const_iterator begin() const noexcept
    {
        return series::begin_impl(m_s_table);
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
        return series::begin_impl(m_s_table);
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
        if (obake_unlikely(!empty())) {
            obake_throw(::std::invalid_argument, "A symbol set can be set only in an empty series, but this series has "
                                                     + detail::to_string(size()) + " terms");
        }

        m_symbol_set = s;
    }

    // Extract a reference to the internal segmented table.
    auto &_get_s_table()
    {
        return m_s_table;
    }
    const auto &_get_s_table() const
    {
        return m_s_table;
    }

    // Reserve enough space for n elements.
    void reserve(size_type n)
    {
        const auto n_tables = s_size_type(1) << m_log2_size;
        const auto n_per_table = static_cast<size_type>(n / n_tables + static_cast<unsigned>((n % n_tables) != 0u));

        for (auto &t : m_s_table) {
            t.reserve(n_per_table);
        }
    }

    template <bool Sign = true,
#if defined(OBAKE_HAVE_CONCEPTS)
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
    void set_n_segments(unsigned l)
    {
        if (obake_unlikely(l > max_log2_size)) {
            obake_throw(::std::invalid_argument, "Cannot set the number of segments to 2**" + detail::to_string(l)
                                                     + ", as this value exceeds the maximum allowed value (2**"
                                                     + detail::to_string(max_log2_size) + ")");
        }

        // NOTE: construct + move assign for exception safety.
        m_s_table = s_table_type(s_size_type(1) << l);
        m_log2_size = l;
    }

    // Remove all the terms in the series.
    // The number of segments and the symbol set will be kept intact.
    void clear_terms() noexcept
    {
        for (auto &t : m_s_table) {
            t.clear();
        }
    }

    // Clear the series.
    // This will remove all the terms and symbols.
    // The number of segments will be kept intact.
    void clear() noexcept
    {
        clear_terms();
        m_symbol_set.clear();
    }

private:
    // Implementation of find(), for both the const and mutable
    // variants.
    template <typename S>
    static auto find_impl(S &s, const K &k)
    {
        // The return type will be iterator/const_iterator, depending on the
        // constness of S.
        using ret_it_t = ::std::conditional_t<::std::is_const_v<S>, const_iterator, iterator>;

        // Helper to find k in a single table belonging
        // to a segmented table st at index idx.
        auto find_single_table = [&s, &k](auto &st, s_size_type idx) {
            assert(idx < st.size());

            // Get out a reference to the table.
            auto &t = st[idx];

            // Find k in the table.
            const auto local_ret = t.find(k);

            if (local_ret == t.end()) {
                // k not found, return the end iterator of
                // the series.
                return s.end();
            } else {
                // k found, build the corresponding
                // series iterator.
                return ret_it_t(&st, idx, local_ret);
            }
        };

        const auto s_table_size = s.m_s_table.size();
        if (s_table_size == 1u) {
            // Single table case.
            return find_single_table(s.m_s_table, 0);
        } else {
            // Segmented table case.
            const auto k_hash = ::obake::hash(k);
            const auto idx = static_cast<s_size_type>(k_hash & (s_table_size - 1u));
            return find_single_table(s.m_s_table, idx);
        }
    }

public:
    // Lookup.
    const_iterator find(const K &k) const
    {
        return series::find_impl(*this, k);
    }
    iterator find(const K &k)
    {
        return series::find_impl(*this, k);
    }

private:
    // Serialisation.
    template <class Archive>
    void save(Archive &ar, unsigned) const
    {
        ar << m_log2_size;
        ar << m_symbol_set;

        for (const auto &tab : m_s_table) {
            ar << tab.size();

            // Save separately key and coefficient.
            for (const auto &t : tab) {
                ar << t.first;
                ar << t.second;
            }
        }
    }
    template <class Archive>
    void load(Archive &ar, unsigned)
    {
        // Empty out this before doing anything.
        clear();

        try {
            // Recover the segmented size.
            unsigned log2_size;
            ar >> log2_size;
            set_n_segments(log2_size);

            // Recover the symbol set.
            ar >> m_symbol_set;

            // Iterate over the tables, reading in
            // the keys/coefficients sequentially
            // and inserting them.
            K tmp_k;
            C tmp_c;
            for (auto &tab : m_s_table) {
                decltype(tab.size()) size;
                ar >> size;
                tab.reserve(size);

                for (decltype(tab.size()) i = 0; i < size; ++i) {
                    ar >> tmp_k;
                    ar >> tmp_c;

                    // NOTE: don't need any checking, as we assume that:
                    // - the original table had no zeroes,
                    // - the original table had no incompatible keys,
                    // - the original table did not overflow the max size,
                    // - the original table had only unique keys.
                    // Note that deserialisation of a series that was saved
                    // in a previous program execution will result in a term
                    // order different from the original one due to abseil's salting,
                    // but the number of terms in a specific table will be the same
                    // because the first level hash is not salted.
                    // NOTE: this is essentially identical to a straight emplace_back()
                    // on the table, just with some added assertions.
                    detail::series_add_term_table<true, detail::sat_check_zero::off, detail::sat_check_compat_key::off,
                                                  detail::sat_check_table_size::off, detail::sat_assume_unique::on>(
                        *this, tab,
                        // NOTE: pass as const, so that we are sure the copy constructors
                        // will be used.
                        ::std::as_const(tmp_k), ::std::as_const(tmp_c));
                }
            }
        } catch (...) {
            // Avoid inconsistent state in case of exceptions.
            clear();
            throw;
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

private:
    s_table_type m_s_table;
    unsigned m_log2_size;
    symbol_set m_symbol_set;
};

// Free function implementation of the swapping primitive.
template <typename K, typename C, typename Tag>
inline void swap(series<K, C, Tag> &s1, series<K, C, Tag> &s2) noexcept
{
    s1.swap(s2);
}

namespace customisation::internal
{

// Metaprogramming to establish the algorithm/return
// type of the default series pow computation.
template <typename T, typename U>
constexpr auto series_default_pow_algorithm_impl()
{
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, detail::type_c<void>{});

    if constexpr (!is_cvr_series_v<T>) {
        // The base is not a series type.
        return failure;
    } else {
        // The coefficient type of T must be:
        // - constructible from int,
        // - exponentiable by U, with the return value a coefficient type
        //   which is constructible from int.
        // The exponent type must be zero-testable.
        using rT = remove_cvref_t<T>;
        using rU = remove_cvref_t<U>;
        using cf_t = series_cf_t<rT>;
        // NOTE: check exponentiability via const
        // lvalue refs.
        using cf_pow_t = detected_t<detail::pow_t, const cf_t &, ::std::add_lvalue_reference_t<const rU>>;

        if constexpr (::std::conjunction_v<::std::is_constructible<cf_t, int>,
                                           // NOTE: these take care of ensuring that cf_pow_t
                                           // is detected (nonesuch is not ctible from int,
                                           // nor it is a coefficient).
                                           is_cf<cf_pow_t>, ::std::is_constructible<cf_pow_t, int>,
                                           is_zero_testable<::std::add_lvalue_reference_t<const rU>>>) {
            return ::std::make_pair(1, detail::type_c<series<series_key_t<rT>, cf_pow_t, series_tag_t<rT>>>{});
        } else {
            return failure;
        }
    }
}

// Default implementation of series exponentiation.
struct series_default_pow_impl {
    // A couple of handy shortcuts.
    template <typename T, typename U>
    static constexpr auto algo_ret = internal::series_default_pow_algorithm_impl<T, U>();

    template <typename T, typename U>
    static constexpr auto algo = series_default_pow_impl::algo_ret<T, U>.first;

    template <typename T, typename U>
    using ret_t = typename decltype(series_default_pow_impl::algo_ret<T, U>.second)::type;

    // Implementation.
    template <typename T, typename U>
    ret_t<T &&, U &&> operator()(T &&b, U &&e_) const
    {
        // Sanity check.
        static_assert(algo<T &&, U &&> != 0);

        using rT = remove_cvref_t<T>;
        using rU = remove_cvref_t<U>;

        // Need only const access to the exponent.
        const auto &e = ::std::as_const(e_);

        if (b.is_single_cf()) {
            // Single coefficient series: either empty (i.e., b is zero),
            // or a single term with unitary key.
            if (b.empty()) {
                // Return 0**e.
                const series_cf_t<rT> zero(0);
                return ret_t<T &&, U &&>(::obake::pow(zero, e));
            } else {
                // Return the only coefficient raised to the exponent.
                return ret_t<T &&, U &&>(::obake::pow(b.cbegin()->second, e));
            }
        }

        // Handle the case of zero exponent: anything to the power
        // of zero is 1.
        if (::obake::is_zero(e)) {
            // NOTE: construct directly from 1: 1 is rank 0,
            // construction then forwards 1 to the construction
            // of an internal coefficient.
            return ret_t<T &&, U &&>(1);
        }

        // Let's determine if we can run exponentiation
        // by repeated multiplications:
        // - e must either be an mppp::integer or safely convertible to
        //   mppp::integer<1>,
        // - the return type must be compound-multipliable by T.
        if constexpr (::std::conjunction_v<::std::disjunction<detail::is_mppp_integer<rU>,
                                                              is_safely_convertible<const rU &, ::mppp::integer<1> &>>,
                                           is_compound_multipliable<ret_t<T &&, U &&> &, const rT &>>) {
            // Transform the exponent into an integral.
            // NOTE: if e is an integer, just return a const reference
            // to it. Otherwise, return a new object of type mppp::integer<1>.
            decltype(auto) n = [&e]() -> decltype(auto) {
                if constexpr (detail::is_mppp_integer_v<rU>) {
                    return e;
                } else {
                    ::mppp::integer<1> ret;

                    if (obake_unlikely(!::obake::safe_convert(ret, e))) {
                        if constexpr (is_stream_insertable_v<const rU &>) {
                            // Provide better error message if U is ostreamable.
                            ::std::ostringstream oss;
                            static_cast<::std::ostream &>(oss) << e;
                            obake_throw(::std::invalid_argument,
                                        "Invalid exponent for series exponentiation via repeated "
                                        "multiplications: the exponent ("
                                            + oss.str() + ") cannot be converted into an integral value");
                        } else {
                            obake_throw(::std::invalid_argument,
                                        "Invalid exponent for series exponentiation via repeated "
                                        "multiplications: the exponent cannot be converted into an integral value");
                        }
                    }

                    return ret;
                }
            }();

            if (obake_unlikely(n.sgn() < 0)) {
                obake_throw(::std::invalid_argument, "Invalid exponent for series exponentiation via repeated "
                                                     "multiplications: the exponent ("
                                                         + n.to_string() + ") is negative");
            }

            // NOTE: constructability from 1 is ensured by the
            // constructability of the coefficient type from int.
            ret_t<T &&, U &&> retval(1);
            for (remove_cvref_t<decltype(n)> i(0); i < n; ++i) {
                retval *= ::std::as_const(b);
            }

            // NOTE: returnability is guaranteed because
            // the return type is a series.
            return retval;
        } else {
            obake_throw(::std::invalid_argument,
                        "Cannot compute the power of a series of type '" + ::obake::type_name<rT>()
                            + "': the series does not consist of a single coefficient, "
                              "and exponentiation via repeated multiplications is not possible (either because the "
                              "exponent cannot be converted to an integral value, or because the series type does "
                              "not support the necessary arithmetic operations)");
        }
    }
};

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
    requires series_default_pow_impl::algo<T, U> != 0 inline constexpr auto pow<T, U>
#else
inline constexpr auto pow<T, U, ::std::enable_if_t<series_default_pow_impl::algo<T, U> != 0>>
#endif
    = series_default_pow_impl{};

} // namespace customisation::internal

// Identity operator for series.
#if defined(OBAKE_HAVE_CONCEPTS)
template <CvrSeries T>
#else
template <typename T, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
#endif
inline remove_cvref_t<T> operator+(T &&x)
{
    return ::std::forward<T>(x);
}

namespace detail
{

// Default implementation of obake::negate() for series.
template <typename T>
inline void series_default_negate_impl(T &&x)
{
    static_assert(is_cvr_series_v<T>);
    for (auto &p : x) {
        // NOTE: the runtime requirements
        // of negate() ensure that the coefficient
        // will never become zero after negation.
        // NOTE: perhaps this can be improved
        // performance-wise by taking advantage
        // of the single-table layout, if possible.
        // NOTE: this could be parallelised,
        // in case of multiple tables.
        ::obake::negate(p.second);
    }
}

} // namespace detail

// Negated copy operator.
#if defined(OBAKE_HAVE_CONCEPTS)
template <CvrSeries T>
#else
template <typename T, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
#endif
inline remove_cvref_t<T> operator-(T &&x)
{
    if constexpr (is_mutable_rvalue_reference_v<T &&>) {
        // NOTE: if we have an rvalue reference in input, we
        // can negate all coefficients in-place and then re-use
        // the same reference to move-construct the return value.
        detail::series_default_negate_impl(::std::forward<T>(x));
        return ::std::forward<T>(x);
    } else {
        // Otherwise, copy and negate.
        auto retval(::std::forward<T>(x));
        detail::series_default_negate_impl(retval);
        return retval;
    }
}

// Customise obake::negate() for series types.
namespace customisation::internal
{

struct series_default_negate_impl {
    template <typename T>
    void operator()(T &&x) const
    {
        detail::series_default_negate_impl(::std::forward<T>(x));
    }
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
    requires CvrSeries<T> && !::std::is_const_v<::std::remove_reference_t<T>> inline constexpr auto negate<T>
#else
inline constexpr auto negate<T, ::std::enable_if_t<::std::conjunction_v<
                                    is_cvr_series<T>, ::std::negation<::std::is_const<::std::remove_reference_t<T>>>>>>
#endif
    = series_default_negate_impl{};

} // namespace customisation::internal

// Customise obake::is_zero() for series types.
namespace customisation::internal
{

struct series_default_is_zero_impl {
    template <typename T>
    bool operator()(const T &x) const
    {
        return x.empty();
    }
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires CvrSeries<T> inline constexpr auto is_zero<T>
#else
inline constexpr auto is_zero<T, ::std::enable_if_t<is_cvr_series_v<T>>>
#endif
    = series_default_is_zero_impl{};

} // namespace customisation::internal

// Customise obake::byte_size() for series types.
namespace customisation::internal
{

struct series_default_byte_size_impl {
    template <typename T>
    ::std::size_t operator()(const T &x) const
    {
        using table_t = remove_cvref_t<decltype(x._get_s_table()[0])>;

        // NOTE: start out with the size of the series class, plus
        // the class size of all the tables. This is slightly incorrect,
        // since, if we only have a single table, its class size will be
        // included in sizeof(T) due to SBO, but it's just a small inaccuracy.
        auto retval = sizeof(T) + x._get_s_table().size() * sizeof(table_t);

        // Add some size for the symbols. This is not 100% accurate
        // due to SBO in strings.
        for (const auto &s : x.get_symbol_set()) {
            // NOTE: s.size() gives the number of chars,
            // thus it's a size in bytes.
            retval += sizeof(::std::string) + s.size();
        }

        for (const auto &tab : x._get_s_table()) {
            // Accumulate the byte size for all terms in the table
            for (const auto &t : tab) {
                // NOTE: old clang does not like structured
                // bindings in the for loop.
                const auto &k = t.first;
                const auto &c = t.second;

                // NOTE: account for possible padding in the series term class.
                static_assert(sizeof(k) + sizeof(c) <= sizeof(series_term_t<T>));
                retval += ::obake::byte_size(k) + ::obake::byte_size(c)
                          + (sizeof(series_term_t<T>) - (sizeof(k) + sizeof(c)));
            }

            // Add the space occupied by the unused slots.
            assert(tab.capacity() >= tab.size());
            retval += (tab.capacity() - tab.size()) * sizeof(series_term_t<T>);
        }

        return retval;
    }
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires CvrSeries<T> &&SizeMeasurable<const series_key_t<remove_cvref_t<T>> &>
    &&SizeMeasurable<const series_cf_t<remove_cvref_t<T>> &> inline constexpr auto byte_size<T>
#else
inline constexpr auto
    byte_size<T, ::std::enable_if_t<
                     ::std::conjunction_v<is_cvr_series<T>, is_size_measurable<const series_key_t<remove_cvref_t<T>> &>,
                                          is_size_measurable<const series_cf_t<remove_cvref_t<T>> &>>>>
#endif
    = series_default_byte_size_impl{};

} // namespace customisation::internal

namespace detail
{

// Implementation of the default streaming for a single term.
OBAKE_DLL_PUBLIC void series_stream_single_term(::std::string &, ::std::string &, const ::std::string &, bool);

// Implementation of the default streaming to os of a series' terms.
template <bool TexMode, typename T>
inline void series_stream_terms_impl(::std::ostream &os, const T &s)
{
    if (s.empty()) {
        // Special-case an empty series.
        os << '0';
        return;
    }

    // Cache access to the symbol set.
    const auto &ss = s.get_symbol_set();

    // Hard-code the limit for now.
    constexpr auto limit = 50ul;

    decltype(s.size()) count = 0;
    auto it = s.begin();
    const auto end = s.end();
    ::std::ostringstream oss;
    ::std::string ret;

    while (it != end && (!limit || count != limit)) {
        // Get the string representations of coefficient and key.
        oss.str("");
        if constexpr (TexMode) {
            ::obake::cf_tex_stream_insert(oss, it->second);
        } else {
            ::obake::cf_stream_insert(oss, it->second);
        }
        auto str_cf = oss.str();

        oss.str("");
        if constexpr (TexMode) {
            ::obake::key_tex_stream_insert(oss, it->first, ss);
        } else {
            ::obake::key_stream_insert(oss, it->first, ss);
        }
        const auto str_key = oss.str();

        // Print the term.
        detail::series_stream_single_term(ret, str_cf, str_key, TexMode);

        // Increase the counters.
        ++count;
        if (++it != end) {
            // Prepare the plus for the next term
            // if we are not at the end.
            ret += '+';
        }
    }

    // If we reached the limit without printing all terms in the series, print the ellipsis.
    if (limit && count == limit && it != end) {
        if constexpr (TexMode) {
            ret += "\\ldots";
        } else {
            ret += "...";
        }
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

// Customise obake::cf_stream_insert() for series types.
namespace customisation::internal
{

struct series_default_cf_stream_insert_impl {
    template <typename T>
    void operator()(::std::ostream &os, const T &x) const
    {
        if (x.size() > 1u) {
            // NOTE: if the series has more than 1 term, bracket it.
            os << '(';
            detail::series_stream_terms_impl<false>(os, x);
            os << ')';
        } else {
            detail::series_stream_terms_impl<false>(os, x);
        }
    }
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires CvrSeries<T> inline constexpr auto cf_stream_insert<T>
#else
inline constexpr auto cf_stream_insert<T, ::std::enable_if_t<is_cvr_series_v<T>>>
#endif
    = series_default_cf_stream_insert_impl{};

} // namespace customisation::internal

// Customise obake::cf_tex_stream_insert() for series types.
namespace customisation::internal
{

// Metaprogramming to establish the algorithm
// of the default cf_tex_stream_insert() implementation
// for series types.
template <typename T>
constexpr auto series_default_cf_tex_stream_insert_algorithm_impl()
{
    if constexpr (!is_cvr_series_v<T>) {
        // Not a series type.
        return 0;
    } else {
        // The coefficient/key types of the series
        // must be suitable for tex stream insertion
        // (via const lvalue refs).
        using rT = remove_cvref_t<T>;

        return static_cast<int>(::std::conjunction_v<is_tex_stream_insertable_cf<const series_cf_t<rT> &>,
                                                     is_tex_stream_insertable_key<const series_key_t<rT> &>>);
    }
}

// Default implementation of cf_tex_stream_insert() for series types.
struct series_default_cf_tex_stream_insert_impl {
    // Shortcut.
    template <typename T>
    static constexpr auto algo = internal::series_default_cf_tex_stream_insert_algorithm_impl<T>();

    template <typename T>
    void operator()(::std::ostream &os, const T &x) const
    {
        if (x.size() > 1u) {
            // NOTE: if the series has more than 1 term, bracket it.
            os << "\\left(";
            detail::series_stream_terms_impl<true>(os, x);
            os << "\\right)";
        } else {
            detail::series_stream_terms_impl<true>(os, x);
        }
    }
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires(series_default_cf_tex_stream_insert_impl::algo<T> != 0) inline constexpr auto cf_tex_stream_insert<T>
#else
inline constexpr auto
    cf_tex_stream_insert<T, ::std::enable_if_t<series_default_cf_tex_stream_insert_impl::algo<T> != 0>>
#endif
    = series_default_cf_tex_stream_insert_impl{};

} // namespace customisation::internal

namespace customisation
{

// External customisation point for obake::series_stream_insert().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
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
    OBAKE_SS_FORWARD_FUNCTION((customisation::series_stream_insert<T &&>)(os, ::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto series_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(series_stream_insert(os, ::std::forward<T>(x)));

// Lowest priority: the default implementation for series.
template <typename T, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
inline void series_stream_insert_impl(::std::ostream &os, T &&s, priority_tag<0>)
{
    using series_t = remove_cvref_t<T>;

    // Print the header.
    os << "Key type        : " << ::obake::type_name<series_key_t<series_t>>() << '\n';
    os << "Coefficient type: " << ::obake::type_name<series_cf_t<series_t>>() << '\n';
    os << "Tag             : " << ::obake::type_name<series_tag_t<series_t>>() << '\n';
    os << "Rank            : " << series_rank<series_t> << '\n';
    os << "Symbol set      : " << detail::to_string(s.get_symbol_set()) << '\n';
    os << "Number of terms : " << s.size() << '\n';

    series_stream_terms_impl<false>(os, s);
}

} // namespace detail

#if defined(_MSC_VER)

struct series_stream_insert_msvc {
    template <typename T>
    constexpr auto operator()(::std::ostream &os, T &&s) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(void(detail::series_stream_insert_impl(os, ::std::forward<T>(s),
                                                                                detail::priority_tag<2>{})))
};

inline constexpr auto series_stream_insert = series_stream_insert_msvc{};

#else

inline constexpr auto series_stream_insert = [](::std::ostream & os, auto &&s) OBAKE_SS_FORWARD_LAMBDA(
    void(detail::series_stream_insert_impl(os, ::std::forward<decltype(s)>(s), detail::priority_tag<2>{})));

#endif

// NOTE: constrain the operator so that it is enabled
// only if s is a series. This way, we avoid it to be too
// greedy.
#if defined(OBAKE_HAVE_CONCEPTS)
template <CvrSeries S>
#else
template <typename S, ::std::enable_if_t<is_cvr_series_v<S>, int> = 0>
#endif
constexpr auto operator<<(::std::ostream &os, S &&s)
    OBAKE_SS_FORWARD_FUNCTION((void(::obake::series_stream_insert(os, ::std::forward<S>(s))), os));

// Customise obake::tex_stream_insert() for series types.
namespace customisation::internal
{

// Default implementation of tex_stream_insert() for series types.
struct series_default_tex_stream_insert_impl {
    // NOTE: the requirements on T are the same as in the default implementation
    // of cf_tex_stream_insert() for series, re-use them.
    template <typename T>
    static constexpr auto algo = series_default_cf_tex_stream_insert_impl::algo<T>;

    template <typename T>
    void operator()(::std::ostream &os, const T &x) const
    {
        detail::series_stream_terms_impl<true>(os, x);
    }
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires(series_default_tex_stream_insert_impl::algo<T> != 0) inline constexpr auto tex_stream_insert<T>
#else
inline constexpr auto tex_stream_insert<T, ::std::enable_if_t<series_default_tex_stream_insert_impl::algo<T> != 0>>
#endif
    = series_default_tex_stream_insert_impl{};

} // namespace customisation::internal

namespace customisation
{

// External customisation point for obake::series_add().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
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
    OBAKE_SS_FORWARD_FUNCTION((customisation::series_add<T &&, U &&>)(::std::forward<T>(x), ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto series_add_impl(T &&x, U &&y, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(series_add(::std::forward<T>(x), ::std::forward<U>(y)));

// Meta-programming to establish the algorithm and return type
// of the default implementation of series add/sub. It will return
// a pair containing an integral value (1, 2 or 3) signalling the algorithm
// to be used in the implementation, and a type_c wrapper representing
// the return type of the operation. If the add/sub implementation is not
// well-defined for the input types, it will return (0, void).
template <bool Sign, typename T, typename U>
constexpr auto series_default_addsub_algorithm_impl()
{
    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    constexpr auto rank_T = series_rank<rT>;
    constexpr auto rank_U = series_rank<rU>;

    // Shortcut for signalling that the add/sub implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, type_c<void>{});

    if constexpr (!rank_T && !rank_U) {
        // Neither T nor U are series, return failure. This will disable
        // the default series_add/sub() implementations.
        return failure;
    } else if constexpr (rank_T < rank_U) {
        // The rank of T is less than the rank of U.
        // Determine the coefficient type of the return series type.
        // NOTE: for the determination of the return type, we use const references.
        // In the implementation, we might also use rvalue refs (but we will check
        // at compile time that we can use them, we will not assume it).
        using ret_cf_t
            = ::std::conditional_t<Sign,
                                   detected_t<add_t, ::std::add_lvalue_reference_t<const rT>, const series_cf_t<rU> &>,
                                   detected_t<sub_t, ::std::add_lvalue_reference_t<const rT>, const series_cf_t<rU> &>>;

        if constexpr (is_cf_v<ret_cf_t>) {
            // The candidate coefficient type is valid. Establish
            // the series return type.
            using ret_t = series<series_key_t<rU>, ret_cf_t, series_tag_t<rU>>;
            // NOTE: we'll have to construct the retval from U,
            // and insert into it a term with coefficient constructed
            // from T.
            if constexpr (::std::conjunction_v<is_constructible<ret_t, U>, is_constructible<ret_cf_t, T>>) {
                return ::std::make_pair(1, type_c<ret_t>{});
            } else {
                return failure;
            }
        } else {
            // The candidate return coefficient type
            // is not valid, or it does not
            // produce a coefficient type. Return failure.
            return failure;
        }
    } else if constexpr (rank_T > rank_U) {
        // Mirror of the above.
        using ret_cf_t
            = ::std::conditional_t<Sign,
                                   detected_t<add_t, const series_cf_t<rT> &, ::std::add_lvalue_reference_t<const rU>>,
                                   detected_t<sub_t, const series_cf_t<rT> &, ::std::add_lvalue_reference_t<const rU>>>;

        if constexpr (is_cf_v<ret_cf_t>) {
            using ret_t = series<series_key_t<rT>, ret_cf_t, series_tag_t<rT>>;
            if constexpr (::std::conjunction_v<is_constructible<ret_t, T>, is_constructible<ret_cf_t, U>>) {
                return ::std::make_pair(2, type_c<ret_t>{});
            } else {
                return failure;
            }
        } else {
            return failure;
        }
    } else {
        // T and U are series with the same rank.
        // Determine the coefficient type of the return type.
        using ret_cf_t = ::std::conditional_t<Sign, detected_t<add_t, const series_cf_t<rT> &, const series_cf_t<rU> &>,
                                              detected_t<sub_t, const series_cf_t<rT> &, const series_cf_t<rU> &>>;

        if constexpr (::std::conjunction_v<::std::is_same<series_key_t<rT>, series_key_t<rU>>,
                                           ::std::is_same<series_tag_t<rT>, series_tag_t<rU>>, is_cf<ret_cf_t>>) {
            // The return cf type is a valid coefficient, and the key and tag of the two series
            // match. Establish the series return type.
            using ret_t = series<series_key_t<rT>, ret_cf_t, series_tag_t<rT>>;

            // In the implementation, we may need to copy/move construct
            // ret_cf_t from the original coefficients. We will use a const
            // ref or a mutable rvalue ref, depending on T/U.
            using cf1_t = ::std::conditional_t<is_mutable_rvalue_reference_v<T &&>, series_cf_t<rT> &&,
                                               const series_cf_t<rT> &>;
            using cf2_t = ::std::conditional_t<is_mutable_rvalue_reference_v<U &&>, series_cf_t<rU> &&,
                                               const series_cf_t<rU> &>;

            if constexpr (::std::conjunction_v<
                              // We may need to construct a ret_t from T or U.
                              is_constructible<ret_t, T>, is_constructible<ret_t, U>,
                              // We may need to copy/move convert the original coefficients
                              // to ret_cf_t.
                              is_constructible<ret_cf_t, cf1_t>, is_constructible<ret_cf_t, cf2_t>,
                              // We may need to merge new symbols into the original key type.
                              // NOTE: the key types of T and U must be identical at the moment,
                              // so checking only T's key type is enough.
                              // NOTE: the merging is done via a const ref.
                              is_symbols_mergeable_key<const series_key_t<rT> &>>) {
                return ::std::make_pair(3, type_c<ret_t>{});
            } else {
                return failure;
            }
        } else {
            return failure;
        }
    }
}

// Shortcuts.
template <bool Sign, typename T, typename U>
inline constexpr auto series_default_addsub_algorithm = detail::series_default_addsub_algorithm_impl<Sign, T, U>();

template <bool Sign, typename T, typename U>
using series_default_addsub_ret_t = typename decltype(series_default_addsub_algorithm<Sign, T, U>.second)::type;

// Default implementation of the add/sub primitive for series.
template <bool Sign, typename T, typename U>
inline series_default_addsub_ret_t<Sign, T &&, U &&> series_default_addsub_impl(T &&x, U &&y)
{
    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    // Determine the algorithm.
    constexpr int algo = series_default_addsub_algorithm<Sign, T &&, U &&>.first;
    static_assert(algo > 0 && algo <= 3);

    // Shortcut to the return type.
    using ret_t = series_default_addsub_ret_t<Sign, T &&, U &&>;

    if constexpr (algo == 1) {
        // The rank of T is less than the rank of U.
        ret_t retval(::std::forward<U>(y));
        if constexpr (!Sign) {
            // For subtraction, negate the return value as
            // we flipped around the operands.
            detail::series_default_negate_impl(retval);
        }

        // NOTE: we can turn off key compat check, as we know
        // the new key will be compatible by construction. The other
        // checks are needed. Also, because we don't know
        // the segmentation of y, we need to use series_add_term()
        // instead of series_add_term_table().
        detail::series_add_term<true, sat_check_zero::on, sat_check_compat_key::off, sat_check_table_size::on,
                                sat_assume_unique::off>(retval, series_key_t<rU>(retval.get_symbol_set()),
                                                        ::std::forward<T>(x));

        return retval;
    } else if constexpr (algo == 2) {
        // The rank of U is less than the rank of T.
        ret_t retval(::std::forward<T>(x));
        detail::series_add_term<Sign, sat_check_zero::on, sat_check_compat_key::off, sat_check_table_size::on,
                                sat_assume_unique::off>(retval, series_key_t<rT>(retval.get_symbol_set()),
                                                        ::std::forward<U>(y));

        return retval;
    } else {
        // Both T and U are series, same rank, possibly different cf.

        // Implementation of the addition/subtraction between
        // two series with identical symbol sets.
        auto merge_with_identical_ss = [](auto &&a, auto &&b) {
            assert(a.get_symbol_set() == b.get_symbol_set());

            // Helper to merge the terms from the smaller series (rhs) into the return value
            // (which will be inited from the larger series, lhs).
            auto term_merger = [](auto &&lhs, auto &&rhs) {
                assert(lhs.size() >= rhs.size());

                using rhs_t = decltype(rhs);

                // Build the retval.
                // NOTE: perhaps in the future we can consider reserving
                // additional space for the return value.
                auto retval = [&lhs, &rhs]() {
                    detail::ignore(rhs);
                    // NOTE: if lhs and rhs are the same object, don't
                    // perfectly forward in order to avoid move-contruction
                    // of retval which would alter the state of rhs.
                    if constexpr (::std::is_same_v<remove_cvref_t<decltype(lhs)>, remove_cvref_t<decltype(rhs)>>) {
                        if (&lhs == &rhs) {
                            return ret_t(lhs);
                        } else {
                            return ret_t(::std::forward<decltype(lhs)>(lhs));
                        }
                    } else {
                        return ret_t(::std::forward<decltype(lhs)>(lhs));
                    }
                }();

                // We may end up moving coefficients from rhs.
                // Make sure we will clear it out properly.
                series_rref_clearer<rhs_t> rhs_c(::std::forward<rhs_t>(rhs));

                // Distinguish the two cases in which the internal table
                // is segmented or not.
                if (retval._get_s_table().size() > 1u) {
                    for (auto &t : rhs) {
                        // NOTE: old clang does not like structured
                        // bindings in the for loop.
                        auto &k = t.first;
                        auto &c = t.second;

                        if constexpr (is_mutable_rvalue_reference_v<rhs_t &&>) {
                            // NOTE: turn on the zero check, as we might end up
                            // annihilating terms during insertion.
                            // Compatibility check is not needed.
                            detail::series_add_term<Sign, sat_check_zero::on, sat_check_compat_key::off,
                                                    sat_check_table_size::on, sat_assume_unique::off>(retval, k,
                                                                                                      ::std::move(c));
                        } else {
                            detail::series_add_term<Sign, sat_check_zero::on, sat_check_compat_key::off,
                                                    sat_check_table_size::on, sat_assume_unique::off>(
                                retval, k, ::std::as_const(c));
                        }
                    }
                } else {
                    assert(retval._get_s_table().size() == 1u);

                    auto &t = retval._get_s_table()[0];

                    for (auto &term : rhs) {
                        // NOTE: old clang does not like structured
                        // bindings in the for loop.
                        auto &k = term.first;
                        auto &c = term.second;

                        if constexpr (is_mutable_rvalue_reference_v<rhs_t &&>) {
                            // NOTE: disable the table size check, as we are
                            // sure we have a single table.
                            detail::series_add_term_table<Sign, sat_check_zero::on, sat_check_compat_key::off,
                                                          sat_check_table_size::off, sat_assume_unique::off>(
                                retval, t, k, ::std::move(c));
                        } else {
                            detail::series_add_term_table<Sign, sat_check_zero::on, sat_check_compat_key::off,
                                                          sat_check_table_size::off, sat_assume_unique::off>(
                                retval, t, k, ::std::as_const(c));
                        }
                    }
                }

                return retval;
            };

            using a_t = decltype(a);
            using b_t = decltype(b);

            if (a.size() >= b.size()) {
                return term_merger(::std::forward<a_t>(a), ::std::forward<b_t>(b));
            } else {
                if constexpr (Sign) {
                    return term_merger(::std::forward<b_t>(b), ::std::forward<a_t>(a));
                } else {
                    // If we flipped around the operands and we are subtracting, we'll have
                    // to negate the result before returning it.
                    auto retval = term_merger(::std::forward<b_t>(b), ::std::forward<a_t>(a));
                    detail::series_default_negate_impl(retval);
                    return retval;
                }
            }
        };

        if (x.get_symbol_set() == y.get_symbol_set()) {
            // Same symbol sets, run the implementation
            // directly on x and y.
            return merge_with_identical_ss(::std::forward<T>(x), ::std::forward<U>(y));
        } else {
            // Merge the symbol sets.
            const auto &[merged_ss, ins_map_x, ins_map_y]
                = detail::merge_symbol_sets(x.get_symbol_set(), y.get_symbol_set());

            // The insertion maps cannot be both empty, as we already handled
            // the identical symbol sets case above.
            assert(!ins_map_x.empty() || !ins_map_y.empty());

            // Create a flag indicating empty insertion maps:
            // - 0 -> both non-empty,
            // - 1 -> x is empty,
            // - 2 -> y is empty.
            // (Cannot both be empty as we handled identical symbol sets already).
            const auto flag
                = static_cast<unsigned>(ins_map_x.empty()) + (static_cast<unsigned>(ins_map_y.empty()) << 1);

            switch (flag) {
                case 1u: {
                    // x already has the correct symbol
                    // set, extend only y.
                    ret_t b;
                    b.set_symbol_set(merged_ss);
                    detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

                    return merge_with_identical_ss(::std::forward<T>(x), ::std::move(b));
                }
                case 2u: {
                    // y already has the correct symbol
                    // set, extend only x.
                    ret_t a;
                    a.set_symbol_set(merged_ss);
                    detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);

                    return merge_with_identical_ss(::std::move(a), ::std::forward<U>(y));
                }
            }

            // Both x and y need to be extended.
            ret_t a, b;
            a.set_symbol_set(merged_ss);
            b.set_symbol_set(merged_ss);
            detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);
            detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

            return merge_with_identical_ss(::std::move(a), ::std::move(b));
        }
    }
}

template <typename T, typename U>
inline constexpr int series_default_add_algo = series_default_addsub_algorithm<true, T, U>.first;

// Lowest priority: the default implementation for series.
template <typename T, typename U, ::std::enable_if_t<series_default_add_algo<T &&, U &&> != 0, int> = 0>
constexpr auto series_add_impl(T &&x, U &&y, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(detail::series_default_addsub_impl<true>(::std::forward<T>(x), ::std::forward<U>(y)));

} // namespace detail

#if defined(_MSC_VER)

struct series_add_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::series_add_impl(::std::forward<T>(x), ::std::forward<U>(y),
                                                                 detail::priority_tag<2>{}))
};

inline constexpr auto series_add = series_add_msvc{};

#else

inline constexpr auto series_add = [](auto &&x, auto &&y) OBAKE_SS_FORWARD_LAMBDA(
    detail::series_add_impl(::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<2>{}));

#endif

// Like with operator<<(), constrain so that the operator
// is enabled only if at least 1 operator is a series.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T> || CvrSeries<U>
#else
template <typename T, typename U, ::std::enable_if_t<::std::disjunction_v<is_cvr_series<T>, is_cvr_series<U>>, int> = 0>
#endif
constexpr auto operator+(T &&x, U &&y)
    OBAKE_SS_FORWARD_FUNCTION(::obake::series_add(::std::forward<T>(x), ::std::forward<U>(y)));

// NOTE: for now, implement operator+=() in terms of operator+().
// This can be optimised later performance-wise.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T>
#else
template <typename T, typename U, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
#endif
    constexpr auto operator+=(T &&x, U &&y)
        OBAKE_SS_FORWARD_FUNCTION(::std::forward<T>(x) = ::std::forward<T>(x) + ::std::forward<U>(y));

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires !CvrSeries<T> && CvrSeries<U>
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<::std::negation<is_cvr_series<T>>, is_cvr_series<U>>, int> = 0>
#endif
    constexpr auto operator+=(T &&x, U &&y) OBAKE_SS_FORWARD_FUNCTION(::std::forward<T>(x) = static_cast<remove_cvref_t<T>>(::std::forward<T>(x) + ::std::forward<U>(y)));

namespace customisation
{

// External customisation point for obake::series_sub().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto series_sub = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto series_sub_impl(T &&x, U &&y, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::series_sub<T &&, U &&>)(::std::forward<T>(x), ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto series_sub_impl(T &&x, U &&y, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(series_sub(::std::forward<T>(x), ::std::forward<U>(y)));

template <typename T, typename U>
inline constexpr int series_default_sub_algo = series_default_addsub_algorithm<false, T, U>.first;

// Lowest priority: the default implementation for series.
template <typename T, typename U, ::std::enable_if_t<series_default_sub_algo<T &&, U &&> != 0, int> = 0>
constexpr auto series_sub_impl(T &&x, U &&y, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(detail::series_default_addsub_impl<false>(::std::forward<T>(x), ::std::forward<U>(y)));

} // namespace detail

#if defined(_MSC_VER)

struct series_sub_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::series_sub_impl(::std::forward<T>(x), ::std::forward<U>(y),
                                                                 detail::priority_tag<2>{}))
};

inline constexpr auto series_sub = series_sub_msvc{};

#else

inline constexpr auto series_sub = [](auto &&x, auto &&y) OBAKE_SS_FORWARD_LAMBDA(
    detail::series_sub_impl(::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<2>{}));

#endif

// Like with operator+(), constrain so that the operator
// is enabled only if at least 1 operator is a series.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T> || CvrSeries<U>
#else
template <typename T, typename U, ::std::enable_if_t<::std::disjunction_v<is_cvr_series<T>, is_cvr_series<U>>, int> = 0>
#endif
constexpr auto operator-(T &&x, U &&y)
    OBAKE_SS_FORWARD_FUNCTION(::obake::series_sub(::std::forward<T>(x), ::std::forward<U>(y)));

// NOTE: for now, implement operator-=() in terms of operator-().
// This can be optimised later performance-wise.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T>
#else
template <typename T, typename U, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
#endif
    constexpr auto operator-=(T &&x, U &&y)
        OBAKE_SS_FORWARD_FUNCTION(::std::forward<T>(x) = ::std::forward<T>(x) - ::std::forward<U>(y));

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires !CvrSeries<T> && CvrSeries<U>
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<::std::negation<is_cvr_series<T>>, is_cvr_series<U>>, int> = 0>
#endif
    constexpr auto operator-=(T &&x, U &&y) OBAKE_SS_FORWARD_FUNCTION(::std::forward<T>(x) = static_cast<remove_cvref_t<T>>(::std::forward<T>(x) - ::std::forward<U>(y)));

namespace customisation
{

// External customisation point for obake::series_mul().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto series_mul = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto series_mul_impl(T &&x, U &&y, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::series_mul<T &&, U &&>)(::std::forward<T>(x), ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto series_mul_impl(T &&x, U &&y, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(series_mul(::std::forward<T>(x), ::std::forward<U>(y)));

// Meta-programming to establish the algorithm and return type
// of the default implementation of series mul. It will return
// a pair containing an integral value (1 or 2) signalling the algorithm
// to be used in the implementation, and a type_c wrapper representing
// the return type of the operation. If the mul implementation is not
// well-defined for the input types, it will return (0, void).
template <typename T, typename U>
constexpr auto series_default_mul_algorithm_impl()
{
    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    constexpr auto rank_T = series_rank<rT>;
    constexpr auto rank_U = series_rank<rU>;

    // Shortcut for signalling that the mul implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, type_c<void>{});

    if constexpr ((!rank_T && !rank_U) || rank_T == rank_U) {
        // Neither T nor U are series, or both are series with the same rank, return failure.
        // This will disable the default mul implementation.
        return failure;
    } else {
        // In all cases, we need to be able to zero-test both operands,
        // via const lvalue refs.
        if constexpr (::std::disjunction_v<
                          ::std::negation<is_zero_testable<::std::add_lvalue_reference_t<const rT>>>,
                          ::std::negation<is_zero_testable<::std::add_lvalue_reference_t<const rU>>>>) {
            return failure;
        } else if constexpr (rank_T < rank_U) {
            // The rank of T is less than the rank of U.
            // Determine the coefficient type of the return series type.
            using ret_cf_t = detected_t<mul_t, ::std::add_lvalue_reference_t<const rT>, const series_cf_t<rU> &>;

            if constexpr (is_cf_v<ret_cf_t>) {
                // The candidate coefficient type is valid. Establish
                // the series return type.
                using ret_t = series<series_key_t<rU>, ret_cf_t, series_tag_t<rU>>;

                // We will need to:
                // - construct the return value from the higher rank type,
                // - multiply in-place the coefficient type of ret_t
                //   by a const reference to T.
                if constexpr (::std::conjunction_v<
                                  is_constructible<ret_t, U>,
                                  is_compound_multipliable<ret_cf_t &, ::std::add_lvalue_reference_t<const rT>>>) {
                    return ::std::make_pair(1, type_c<ret_t>{});
                } else {
                    return failure;
                }
            } else {
                return failure;
            }
        } else {
            // Mirror of the above.
            using ret_cf_t = detected_t<mul_t, const series_cf_t<rT> &, ::std::add_lvalue_reference_t<const rU>>;
            if constexpr (is_cf_v<ret_cf_t>) {
                using ret_t = series<series_key_t<rT>, ret_cf_t, series_tag_t<rT>>;
                if constexpr (::std::conjunction_v<
                                  is_constructible<ret_t, T>,
                                  is_compound_multipliable<ret_cf_t &, ::std::add_lvalue_reference_t<const rU>>>) {
                    return ::std::make_pair(2, type_c<ret_t>{});
                } else {
                    return failure;
                }
            } else {
                return failure;
            }
        }
    }
}

// Shortcuts.
template <typename T, typename U>
inline constexpr auto series_default_mul_algorithm = detail::series_default_mul_algorithm_impl<T, U>();

template <typename T, typename U>
inline constexpr int series_default_mul_algo = series_default_mul_algorithm<T, U>.first;

template <typename T, typename U>
using series_default_mul_ret_t = typename decltype(series_default_mul_algorithm<T, U>.second)::type;

// Default implementation of the mul primitive for series.
template <typename T, typename U>
inline series_default_mul_ret_t<T &&, U &&> series_default_mul_impl(T &&x, U &&y)
{
    // Determine the algorithm.
    constexpr int algo = series_default_mul_algo<T &&, U &&>;
    static_assert(algo > 0 && algo <= 2);

    // Shortcut to the return type.
    using ret_t = series_default_mul_ret_t<T &&, U &&>;

    // Helper to implement the multiplication of a series
    // of a higher rank (a) by a series with a lower rank (b);
    auto impl = [](auto &&a, auto &&b) {
        using ra_t = remove_cvref_t<decltype(a)>;
        using rb_t = remove_cvref_t<decltype(b)>;

        static_assert((series_rank<ra_t>) > series_rank<rb_t>);

        // If either a or b is zero, return an
        // empty series.
        if (::obake::is_zero(::std::as_const(a)) || ::obake::is_zero(::std::as_const(b))) {
            return ret_t{};
        }

        // Init the return value from the higher-rank series.
        ret_t retval(::std::forward<decltype(a)>(a));

        // Multiply in-place all coefficients of retval by b.
        // Store in a vector the keys of the terms
        // whose coefficients become zero after
        // the multiplication, so that we can remove them.
        // NOTE: this can be parallelised for a segmented table.
        auto &s_table = retval._get_s_table();
        ::std::vector<series_key_t<ret_t>> v_keys;
        try {
            for (auto &t : s_table) {
                // Reset the vector of keys to be removed.
                v_keys.clear();

                // Perform the multiplications for this table.
                for (auto &term : t) {
                    // NOTE: old clang does not like structured
                    // bindings in the for loop.
                    auto &k = term.first;
                    auto &c = term.second;

                    c *= ::std::as_const(b);
                    // Check if the coefficient became zero
                    // after the multiplication.
                    if (obake_unlikely(::obake::is_zero(::std::as_const(c)))) {
                        v_keys.push_back(k);
                    }
                }

                // Erase any term whose coefficient became zero.
                for (const auto &k : v_keys) {
                    assert(t.find(k) != t.end());
                    t.erase(k);
                }
            }

            return retval;
            // LCOV_EXCL_START
        } catch (...) {
            // If something goes wrong, make sure to clear
            // out retval before rethrowing, in order to avoid
            // a possibly inconsistent state and thus assertion
            // failures in debug mode.
            retval.clear();
            throw;
        }
        // LCOV_EXCL_STOP
    };

    if constexpr (algo == 2) {
        return impl(::std::forward<T>(x), ::std::forward<U>(y));
    } else {
        return impl(::std::forward<U>(y), ::std::forward<T>(x));
    }
}

// Lowest priority: the default implementation for series.
template <typename T, typename U, ::std::enable_if_t<series_default_mul_algo<T &&, U &&> != 0, int> = 0>
constexpr auto series_mul_impl(T &&x, U &&y, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(detail::series_default_mul_impl(::std::forward<T>(x), ::std::forward<U>(y)));

} // namespace detail

#if defined(_MSC_VER)

struct series_mul_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::series_mul_impl(::std::forward<T>(x), ::std::forward<U>(y),
                                                                 detail::priority_tag<2>{}))
};

inline constexpr auto series_mul = series_mul_msvc{};

#else

inline constexpr auto series_mul = [](auto &&x, auto &&y) OBAKE_SS_FORWARD_LAMBDA(
    detail::series_mul_impl(::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<2>{}));

#endif

// Like with operator+(), constrain so that the operator
// is enabled only if at least 1 operator is a series.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T> || CvrSeries<U>
#else
template <typename T, typename U, ::std::enable_if_t<::std::disjunction_v<is_cvr_series<T>, is_cvr_series<U>>, int> = 0>
#endif
constexpr auto operator*(T &&x, U &&y)
    OBAKE_SS_FORWARD_FUNCTION(::obake::series_mul(::std::forward<T>(x), ::std::forward<U>(y)));

// NOTE: for now, implement operator*=() in terms of operator*().
// This can be optimised later performance-wise.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T>
#else
template <typename T, typename U, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
#endif
    constexpr auto operator*=(T &&x, U &&y)
        OBAKE_SS_FORWARD_FUNCTION(::std::forward<T>(x) = ::std::forward<T>(x) * ::std::forward<U>(y));

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires !CvrSeries<T> && CvrSeries<U>
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<::std::negation<is_cvr_series<T>>, is_cvr_series<U>>, int> = 0>
#endif
    constexpr auto operator*=(T &&x, U &&y) OBAKE_SS_FORWARD_FUNCTION(::std::forward<T>(x) = static_cast<remove_cvref_t<T>>(::std::forward<T>(x) * ::std::forward<U>(y)));

namespace customisation
{

// External customisation point for obake::series_div().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto series_div = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto series_div_impl(T &&x, U &&y, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::series_div<T &&, U &&>)(::std::forward<T>(x), ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto series_div_impl(T &&x, U &&y, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(series_div(::std::forward<T>(x), ::std::forward<U>(y)));

// Meta-programming to establish the algorithm and return type
// of the default implementation of series div. It will return
// a pair containing an integral value signalling the algorithm
// to be used in the implementation, and a type_c wrapper representing
// the return type of the operation. If the div implementation is not
// well-defined for the input types, it will return (0, void).
template <typename T, typename U>
constexpr auto series_default_div_algorithm_impl()
{
    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    constexpr auto rank_T = series_rank<rT>;
    constexpr auto rank_U = series_rank<rU>;

    // Shortcut for signalling that the div implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, type_c<void>{});

    if constexpr (rank_T > rank_U) {
        // The candidate coefficient type of the quotient.
        using ret_cf_t = detected_t<div_t, const series_cf_t<rT> &, ::std::add_lvalue_reference_t<const rU>>;

        if constexpr (is_cf_v<ret_cf_t>) {
            // The candidate coefficient type is valid. Establish
            // the series return type.
            using ret_t = series<series_key_t<rT>, ret_cf_t, series_tag_t<rT>>;

            // We will need to:
            // - construct the return value from T,
            // - divide in-place the coefficient type of ret_t
            //   by a const reference to U.
            if constexpr (::std::conjunction_v<
                              is_constructible<ret_t, T>,
                              is_compound_divisible<ret_cf_t &, ::std::add_lvalue_reference_t<const rU>>>) {
                return ::std::make_pair(1, type_c<ret_t>{});
            } else {
                return failure;
            }
        } else {
            // Non-coefficient type resulting from the division.
            return failure;
        }
    } else {
        // The rank of T is not greater than the rank of U. Return failure.
        return failure;
    }
}

// Shortcuts.
template <typename T, typename U>
inline constexpr auto series_default_div_algorithm = detail::series_default_div_algorithm_impl<T, U>();

template <typename T, typename U>
inline constexpr int series_default_div_algo = series_default_div_algorithm<T, U>.first;

template <typename T, typename U>
using series_default_div_ret_t = typename decltype(series_default_div_algorithm<T, U>.second)::type;

// Default implementation of the div primitive for series.
template <typename T, typename U>
inline series_default_div_ret_t<T &&, U &&> series_default_div_impl(T &&x, U &&y)
{
    // Sanity check the algo.
    static_assert(series_default_div_algo<T &&, U &&> == 1);

    // Shortcut to the return type.
    using ret_t = series_default_div_ret_t<T &&, U &&>;

    // Init the return value from the higher-rank series.
    ret_t retval(::std::forward<T>(x));

    // Divide in-place all coefficients of retval by y.
    // NOTE: this can be parallelised for a segmented table.
    auto &s_table = retval._get_s_table();
    try {
        for (auto &t : s_table) {
            // Perform the divisions for this table.
            const auto end = t.end();
            for (auto it = t.begin(); it != end;) {
                auto &c = it->second;

                c /= ::std::as_const(y);

                if (obake_unlikely(::obake::is_zero(::std::as_const(c)))) {
                    // NOTE: abseil's flat_hash_map returns void on erase(),
                    // thus we need to increase 'it' before possibly erasing.
                    // erase() does not cause rehash and thus will not invalidate
                    // any other iterator apart from the one being erased.
                    t.erase(it++);
                } else {
                    ++it;
                }
            }
        }

        return retval;
    } catch (...) {
        // If something goes wrong, make sure to clear
        // out retval before rethrowing, in order to avoid
        // a possibly inconsistent state and thus assertion
        // failures in debug mode.
        retval.clear();
        throw;
    }
}

// Lowest priority: the default implementation for series.
template <typename T, typename U, ::std::enable_if_t<series_default_div_algo<T &&, U &&> != 0, int> = 0>
constexpr auto series_div_impl(T &&x, U &&y, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(detail::series_default_div_impl(::std::forward<T>(x), ::std::forward<U>(y)));

} // namespace detail

#if defined(_MSC_VER)

struct series_div_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::series_div_impl(::std::forward<T>(x), ::std::forward<U>(y),
                                                                 detail::priority_tag<2>{}))
};

inline constexpr auto series_div = series_div_msvc{};

#else

inline constexpr auto series_div = [](auto &&x, auto &&y) OBAKE_SS_FORWARD_LAMBDA(
    detail::series_div_impl(::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<2>{}));

#endif

// Like with operator+(), constrain so that the operator
// is enabled only if at least 1 operator is a series.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T> || CvrSeries<U>
#else
template <typename T, typename U, ::std::enable_if_t<::std::disjunction_v<is_cvr_series<T>, is_cvr_series<U>>, int> = 0>
#endif
constexpr auto operator/(T &&x, U &&y)
    OBAKE_SS_FORWARD_FUNCTION(::obake::series_div(::std::forward<T>(x), ::std::forward<U>(y)));

// NOTE: for now, implement operator/=() in terms of operator/().
// This can be optimised later performance-wise.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T>
#else
template <typename T, typename U, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
#endif
    constexpr auto operator/=(T &&x, U &&y)
        OBAKE_SS_FORWARD_FUNCTION(::std::forward<T>(x) = ::std::forward<T>(x) / ::std::forward<U>(y));

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires !CvrSeries<T> && CvrSeries<U>
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<::std::negation<is_cvr_series<T>>, is_cvr_series<U>>, int> = 0>
#endif
    constexpr auto operator/=(T &&x, U &&y) OBAKE_SS_FORWARD_FUNCTION(::std::forward<T>(x) = static_cast<remove_cvref_t<T>>(::std::forward<T>(x) / ::std::forward<U>(y)));

namespace customisation
{

// External customisation point for obake::series_equal_to().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto series_equal_to = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto series_equal_to_impl(T &&x, U &&y, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::series_equal_to<T &&, U &&>)(::std::forward<T>(x), ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto series_equal_to_impl(T &&x, U &&y, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(series_equal_to(::std::forward<T>(x), ::std::forward<U>(y)));

template <typename T, typename U>
constexpr int series_equal_to_algorithm_impl()
{
    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    constexpr auto rank_T = series_rank<rT>;
    constexpr auto rank_U = series_rank<rU>;

    if constexpr (!rank_T && !rank_U) {
        // Neither T nor U are series, return 0. This will disable
        // the default series_equal_to() implementation.
        return 0;
    } else if constexpr (rank_T < rank_U) {
        // The rank of T is less than the rank of U. We need to be able to:
        // - zero-test const T &,
        // - compare via lvalue const refs T and the coefficient
        //   type of U.
        return ::std::conjunction_v<
                   is_zero_testable<::std::add_lvalue_reference_t<const rT>>,
                   is_equality_comparable<::std::add_lvalue_reference_t<const rT>, const series_cf_t<rU> &>>
                   ? 1
                   : 0;
    } else if constexpr (rank_T > rank_U) {
        // Mirror of the above.
        return ::std::conjunction_v<
                   is_zero_testable<::std::add_lvalue_reference_t<const rU>>,
                   is_equality_comparable<::std::add_lvalue_reference_t<const rU>, const series_cf_t<rT> &>>
                   ? 2
                   : 0;
    } else {
        // T and U are series with the same rank.
        if constexpr (::std::conjunction_v<::std::is_same<series_key_t<rT>, series_key_t<rU>>,
                                           ::std::is_same<series_tag_t<rT>, series_tag_t<rU>>>) {
            // The key and tag of the two series match.
            return ::std::conjunction_v<
                       // We may need to merge new symbols into the original key type.
                       // NOTE: in the comparison implementation, we are only
                       // extending the key but not converting coefficients,
                       // so no need to check for coefficients convertibility.
                       is_symbols_mergeable_key<const series_key_t<rT> &>,
                       // We need to compare the coefficient types, via const lvalue refs.
                       is_equality_comparable<const series_cf_t<rT> &, const series_cf_t<rU> &>>
                       ? 3
                       : 0;
        } else {
            return 0;
        }
    }
}

template <typename T, typename U>
inline constexpr int series_equal_to_algorithm = detail::series_equal_to_algorithm_impl<T, U>();

// Lowest priority: the default implementation for series.
template <typename T, typename U, ::std::enable_if_t<series_equal_to_algorithm<T &&, U &&> != 0, int> = 0>
constexpr bool series_equal_to_impl(T &&x, U &&y, priority_tag<0>)
{
    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    constexpr int algo = series_equal_to_algorithm<T &&, U &&>;
    static_assert(algo > 0 && algo <= 3);

    if constexpr (algo == 3) {
        // Two series with equal rank, same key/tag, possibly
        // different coefficient type.

        // Helper to compare series with identical symbol sets.
        auto cmp_identical_ss = [](const auto &lhs, const auto &rhs) {
            assert(lhs.get_symbol_set() == rhs.get_symbol_set());

            // If the series have different sizes,
            // they cannot be equal.
            if (lhs.size() != rhs.size()) {
                return false;
            }

            // Cache the end iterator of rhs.
            const auto rhs_end = rhs.end();
            // NOTE: this can be parallelised and
            // improved performance-wise by having specialised
            // codepaths for single table layout, same
            // n segments, etc. Keep it in mind for
            // future optimisations.
            for (const auto &t : lhs) {
                // NOTE: old clang does not like structured
                // bindings in the for loop.
                const auto &k = t.first;
                const auto &c = t.second;

                const auto it = rhs.find(k);
                if (it == rhs_end || c != it->second) {
                    return false;
                }
            }

            return true;
        };

        if (x.get_symbol_set() == y.get_symbol_set()) {
            return cmp_identical_ss(x, y);
        } else {
            // Merge the symbol sets.
            const auto &[merged_ss, ins_map_x, ins_map_y]
                = detail::merge_symbol_sets(x.get_symbol_set(), y.get_symbol_set());

            // The insertion maps cannot be both empty, as we already handled
            // the identical symbol sets case above.
            assert(!ins_map_x.empty() || !ins_map_y.empty());

            // Create a flag indicating empty insertion maps:
            // - 0 -> both non-empty,
            // - 1 -> x is empty,
            // - 2 -> y is empty.
            // (Cannot both be empty as we handled identical symbol sets already).
            const auto flag
                = static_cast<unsigned>(ins_map_x.empty()) + (static_cast<unsigned>(ins_map_y.empty()) << 1);

            switch (flag) {
                case 1u: {
                    // x already has the correct symbol
                    // set, extend only y.
                    rU b;
                    b.set_symbol_set(merged_ss);
                    detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

                    return cmp_identical_ss(x, b);
                }
                case 2u: {
                    // y already has the correct symbol
                    // set, extend only x.
                    rT a;
                    a.set_symbol_set(merged_ss);
                    detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);

                    return cmp_identical_ss(a, y);
                }
            }

            // Both x and y need to be extended.
            rT a;
            rU b;
            a.set_symbol_set(merged_ss);
            b.set_symbol_set(merged_ss);
            detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);
            detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

            return cmp_identical_ss(a, b);
        }
    } else {
        // Helper to compare series of different rank
        // (lhs has higher rank than rhs).
        auto diff_rank_cmp = [](const auto &lhs, const auto &rhs) {
            static_assert((series_rank<remove_cvref_t<decltype(lhs)>>) > series_rank<remove_cvref_t<decltype(rhs)>>);

            switch (lhs.size()) {
                case 0u:
                    // An empty series is considered equal to zero.
                    return ::obake::is_zero(rhs);
                case 1u: {
                    // lhs has a single term: if its key is unitary
                    // and its coefficient equal to rhs, return true,
                    // otherwise return false.
                    const auto it = lhs.begin();
                    return ::obake::key_is_one(it->first, lhs.get_symbol_set()) && it->second == rhs;
                }
                default:
                    // lhs has more than 1 term, return false.
                    return false;
            }
        };

        if constexpr (algo == 1) {
            // The rank of T is less than the rank of U.
            return diff_rank_cmp(y, x);
        } else {
            // The rank of U is less than the rank of T.
            return diff_rank_cmp(x, y);
        }
    }
}

} // namespace detail

#if defined(_MSC_VER)

struct series_equal_to_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y) const OBAKE_SS_FORWARD_MEMBER_FUNCTION(static_cast<bool>(
        detail::series_equal_to_impl(::std::forward<T>(x), ::std::forward<U>(y), detail::priority_tag<2>{})))
};

inline constexpr auto series_equal_to = series_equal_to_msvc{};

#else

// NOTE: forcibly cast to bool the return value, so that if the selected implementation
// returns a type which is not convertible to bool, this call will SFINAE out.
inline constexpr auto series_equal_to =
    [](auto &&x, auto &&y) OBAKE_SS_FORWARD_LAMBDA(static_cast<bool>(detail::series_equal_to_impl(
        ::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<2>{})));

#endif

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T> || CvrSeries<U>
#else
template <typename T, typename U, ::std::enable_if_t<::std::disjunction_v<is_cvr_series<T>, is_cvr_series<U>>, int> = 0>
#endif
constexpr auto operator==(T &&x, U &&y)
    OBAKE_SS_FORWARD_FUNCTION(::obake::series_equal_to(::std::forward<T>(x), ::std::forward<U>(y)));

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T> || CvrSeries<U>
#else
template <typename T, typename U, ::std::enable_if_t<::std::disjunction_v<is_cvr_series<T>, is_cvr_series<U>>, int> = 0>
#endif
constexpr auto operator!=(T &&x, U &&y)
    OBAKE_SS_FORWARD_FUNCTION(!::obake::series_equal_to(::std::forward<T>(x), ::std::forward<U>(y)));

// Customise obake::degree() for series types.
namespace customisation::internal
{

// Metaprogramming to establish the algorithm/return
// type of the default series degree computation. This is shared
// between total and partial degree computation, since the
// logic is identical and the only changing parts are the
// typedef/type traits to establish if the key/cf support
// the degree computation.
template <typename T, template <typename> typename IsWithDegree, template <typename> typename IsKeyWithDegree,
          template <typename> typename DegreeT, template <typename> typename KeyDegreeT>
constexpr auto series_default_degree_algorithm_impl()
{
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, detail::type_c<void>{});

    if constexpr (!is_cvr_series_v<T>) {
        // Not a series type.
        return failure;
    } else {
        using rT = remove_cvref_t<T>;
        using cf_t = series_cf_t<rT>;
        using key_t = series_key_t<rT>;

        constexpr bool cf_has_degree = IsWithDegree<const cf_t &>::value;
        constexpr bool key_has_degree = IsKeyWithDegree<const key_t &>::value;

        if constexpr (!cf_has_degree && !key_has_degree) {
            // Neither key nor cf are with degree.
            return failure;
        } else {
            if constexpr (cf_has_degree && key_has_degree) {
                // Both key and cf are with degree.
                using cf_degree_t = DegreeT<const cf_t &>;
                using key_degree_t = KeyDegreeT<const key_t &>;

                // NOTE: check for addability using rvalues.
                using degree_t = detected_t<detail::add_t, key_degree_t, cf_degree_t>;

                if constexpr (::std::conjunction_v<
                                  // NOTE: these take care of ensuring that degree_t is detected
                                  // (because nonesuch is not lt-comparable etc.)
                                  is_less_than_comparable<::std::add_lvalue_reference_t<const degree_t>>,
                                  ::std::is_constructible<degree_t, int>, is_returnable<degree_t>,
                                  // NOTE: require a semi-regular type,
                                  // it's just easier to reason about.
                                  is_semi_regular<degree_t>>) {
                    // degree_t is well defined, it supports operator<, it can be constructed from int,
                    // it is returnable and semi-regular (hence, move-assignable).
                    return ::std::make_pair(1, detail::type_c<degree_t>{});
                } else {
                    return failure;
                }
            } else if constexpr (cf_has_degree) {
                // Only the coefficient is with degree.
                using degree_t = DegreeT<const cf_t &>;

                if constexpr (::std::conjunction_v<
                                  is_less_than_comparable<::std::add_lvalue_reference_t<const degree_t>>,
                                  ::std::is_constructible<degree_t, int>, is_returnable<degree_t>,
                                  // NOTE: require a semi-regular type,
                                  // it's just easier to reason about.
                                  is_semi_regular<degree_t>>) {
                    // degree_t supports operator<, it can be constructed from int,
                    // it is returnable and semi-regular (hence, move-assignable).
                    return ::std::make_pair(2, detail::type_c<degree_t>{});
                } else {
                    return failure;
                }
            } else {
                // Only the key is with degree.
                using degree_t = KeyDegreeT<const key_t &>;

                if constexpr (::std::conjunction_v<
                                  is_less_than_comparable<::std::add_lvalue_reference_t<const degree_t>>,
                                  ::std::is_constructible<degree_t, int>, is_returnable<degree_t>,
                                  // NOTE: require a semi-regular type,
                                  // it's just easier to reason about.
                                  is_semi_regular<degree_t>>) {
                    // degree_t supports operator<, it can be constructed from int,
                    // it is returnable and semi-regular (hence, move-assignable).
                    return ::std::make_pair(3, detail::type_c<degree_t>{});
                } else {
                    return failure;
                }
            }
        }
    }
}

struct series_default_degree_impl {
    // A couple of handy shortcuts.
    template <typename T>
    static constexpr auto algo_ret
        = internal::series_default_degree_algorithm_impl<T, is_with_degree, is_key_with_degree, detail::degree_t,
                                                         detail::key_degree_t>();

    template <typename T>
    static constexpr auto algo = series_default_degree_impl::algo_ret<T>.first;

    template <typename T>
    using ret_t = typename decltype(series_default_degree_impl::algo_ret<T>.second)::type;

    // Helper to extract the degree of a term p.
    // NOTE: here T is used only in the selection of the
    // algorithm, which does not depend on cvref qualifiers
    // for T. Thus, this functor will produce the same results
    // regardless of the cvref qualifications on T.
    template <typename T>
    struct d_extractor {
        template <typename U>
        auto operator()(const U &p) const
        {
            constexpr auto al = algo<remove_cvref_t<T>>;
            static_assert(al > 0 && al <= 3);
            assert(ss != nullptr);

            if constexpr (al == 1) {
                // Both coefficient and key with degree.
                return ::obake::key_degree(p.first, *ss) + ::obake::degree(p.second);
            } else if constexpr (al == 2) {
                // Only coefficient with degree.
                return ::obake::degree(p.second);
            } else {
                // Only key with degree.
                return ::obake::key_degree(p.first, *ss);
            }
        }
        template <typename U>
        auto operator()(const U *p) const
        {
            return operator()(*p);
        }
        const symbol_set *ss = nullptr;
    };

    // Implementation.
    template <typename T>
    ret_t<T &&> operator()(T &&x_) const
    {
        // We just need const access to x.
        const auto &x = ::std::as_const(x_);

        // Special case for an empty series.
        if (x.empty()) {
            return ret_t<T &&>(0);
        }

        // The functor to extract the term's degree.
        d_extractor<T &&> d_extract{&x.get_symbol_set()};

        // Find the maximum degree.
        // NOTE: parallelisation opportunities here for
        // segmented tables.
        auto it = x.cbegin();
        const auto end = x.cend();
        ret_t<T &&> max_deg(d_extract(*it));
        for (++it; it != end; ++it) {
            ret_t<T &&> cur(d_extract(*it));
            if (::std::as_const(max_deg) < ::std::as_const(cur)) {
                max_deg = ::std::move(cur);
            }
        }

        return max_deg;
    }
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
    requires series_default_degree_impl::algo<T> != 0 inline constexpr auto degree<T>
#else
inline constexpr auto degree<T, ::std::enable_if_t<series_default_degree_impl::algo<T> != 0>>
#endif
    = series_default_degree_impl{};

} // namespace customisation::internal

// Customise obake::p_degree() for series types.
namespace customisation::internal
{

struct series_default_p_degree_impl {
    // A couple of handy shortcuts.
    template <typename T>
    static constexpr auto algo_ret
        = internal::series_default_degree_algorithm_impl<T, is_with_p_degree, is_key_with_p_degree, detail::p_degree_t,
                                                         detail::key_p_degree_t>();

    template <typename T>
    static constexpr auto algo = series_default_p_degree_impl::algo_ret<T>.first;

    template <typename T>
    using ret_t = typename decltype(series_default_p_degree_impl::algo_ret<T>.second)::type;

    // Helper to extract the partial degree of a term p.
    // NOTE: here T is used only in the selection of the
    // algorithm, which does not depend on cvref qualifiers
    // for T. Thus, this functor will produce the same results
    // regardless of the cvref qualifications on T.
    template <typename T>
    struct d_extractor {
        template <typename U>
        auto operator()(const U &p) const
        {
            constexpr auto al = algo<remove_cvref_t<T>>;
            static_assert(al > 0 && al <= 3);
            assert(s != nullptr);
            assert(si != nullptr);
            assert(ss != nullptr);

            if constexpr (al == 1) {
                // Both coefficient and key with partial degree.
                return ::obake::key_p_degree(p.first, *si, *ss) + ::obake::p_degree(p.second, *s);
            } else if constexpr (al == 2) {
                // Only coefficient with partial degree.
                return ::obake::p_degree(p.second, *s);
            } else {
                // Only key with degree.
                return ::obake::key_p_degree(p.first, *si, *ss);
            }
        }
        template <typename U>
        auto operator()(const U *p) const
        {
            return operator()(*p);
        }
        const symbol_set *s = nullptr;
        const symbol_idx_set *si = nullptr;
        const symbol_set *ss = nullptr;
    };

    // Implementation.
    template <typename T>
    ret_t<T &&> operator()(T &&x_, const symbol_set &s) const
    {
        // We just need const access to x.
        const auto &x = ::std::as_const(x_);

        // Special case for an empty series.
        if (x.empty()) {
            return ret_t<T &&>(0);
        }

        // Cache x's symbol set.
        const auto &ss = x.get_symbol_set();

        // Fetch the indices of s in ss.
        // NOTE: this computation could be avoided in case the
        // key type has no degree.
        const auto si = detail::ss_intersect_idx(s, ss);

        // The functor to extract the term's partial degree.
        d_extractor<T &&> d_extract{&s, &si, &ss};

        // Find the maximum degree.
        // NOTE: parallelisation opportunities here for
        // segmented tables.
        auto it = x.cbegin();
        const auto end = x.cend();
        ret_t<T &&> max_deg(d_extract(*it));
        for (++it; it != end; ++it) {
            ret_t<T &&> cur(d_extract(*it));
            if (::std::as_const(max_deg) < ::std::as_const(cur)) {
                max_deg = ::std::move(cur);
            }
        }

        return max_deg;
    }
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
    requires series_default_p_degree_impl::algo<T> != 0 inline constexpr auto p_degree<T>
#else
inline constexpr auto p_degree<T, ::std::enable_if_t<series_default_p_degree_impl::algo<T> != 0>>
#endif
    = series_default_p_degree_impl{};

} // namespace customisation::internal

namespace customisation::internal
{

// Metaprogramming to establish the algorithm/return
// type of the default series evaluate computation.
template <typename T, typename U>
constexpr auto series_default_evaluate_algorithm_impl()
{
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, detail::type_c<void>{});

    if constexpr (!is_cvr_series_v<T>) {
        // Not a series type.
        return failure;
    } else {
        using rT = remove_cvref_t<T>;
        using key_t = series_key_t<rT>;
        using cf_t = series_cf_t<rT>;

        // The candidate return type is the type of the product
        // of the evaluations of key and coefficient. The evaluations
        // are done via const lvalue refs, the product via rvalues.
        if constexpr (::std::conjunction_v<is_evaluable_key<const key_t &, U>, is_evaluable<const cf_t &, U>>) {
            using key_eval_t = detail::key_evaluate_t<const key_t &, U>;
            using key_cf_t = detail::evaluate_t<const cf_t &, U>;

            // The candidate return type. It needs to be in-place addable (lvalue += rvalue),
            // constructible from int and returnable.
            using ret_t = detected_t<detail::mul_t, key_eval_t, key_cf_t>;

            if constexpr (::std::conjunction_v<
                              // NOTE: these will also verify that ret_t is detected.
                              is_compound_addable<::std::add_lvalue_reference_t<ret_t>, ret_t>,
                              ::std::is_constructible<ret_t, int>, is_returnable<ret_t>,
                              // NOTE: require a semi-regular type,
                              // it's just easier to reason about.
                              is_semi_regular<ret_t>>) {
                return ::std::make_pair(1, detail::type_c<ret_t>{});
            } else {
                return failure;
            }
        } else {
            return failure;
        }
    }
}

// Default implementation of series evaluation.
struct series_default_evaluate_impl {
    // A couple of handy shortcuts.
    template <typename T, typename U>
    static constexpr auto algo_ret = internal::series_default_evaluate_algorithm_impl<T, U>();

    template <typename T, typename U>
    static constexpr auto algo = series_default_evaluate_impl::algo_ret<T, U>.first;

    template <typename T, typename U>
    using ret_t = typename decltype(series_default_evaluate_impl::algo_ret<T, U>.second)::type;

    // Implementation.
    template <typename T, typename U>
    ret_t<T &&, U> operator()(T &&s_, const symbol_map<U> &sm) const
    {
        // Need only const access to s.
        const auto &s = ::std::as_const(s_);

        // Cache the symbol set.
        const auto &ss = s.get_symbol_set();

        // Compute the intersection between sm and ss.
        const auto si = detail::sm_intersect_idx(sm, ss);

        if (obake_unlikely(si.size() != ss.size())) {
            // If the intersection does not contain the same
            // number of elements as ss, then it means that
            // elements in ss are missing from sm.

            // Helper to extract a string reference from an item in sm.
            struct str_extractor {
                const ::std::string &operator()(const typename symbol_map<U>::value_type &p) const
                {
                    return p.first;
                }
            };

            obake_throw(
                ::std::invalid_argument,
                "Cannot evaluate a series: the evaluation map, which contains the symbols "
                    + detail::to_string(symbol_set(::boost::container::ordered_unique_range_t{},
                                                   ::boost::make_transform_iterator(sm.cbegin(), str_extractor{}),
                                                   ::boost::make_transform_iterator(sm.cend(), str_extractor{})))
                    + ", does not contain all the symbols in the series' symbol set, " + detail::to_string(ss));
        }

        // si represents an intersection between sm and ss,
        // and we know that here si.size() == ss.size(),
        // Thus, si must contain the [0, ss.size()) sequence.
        assert(si.empty() || (si.cend() - 1)->first == (ss.size() - 1u));

        // NOTE: parallelisation opportunities here.
        ret_t<T &&, U> retval(0);
        for (const auto &tab : s._get_s_table()) {
            for (const auto &t : tab) {
                const auto &k = t.first;
                const auto &c = t.second;

                // NOTE: there's an opportunity for fma3 here,
                // but I am not sure it's worth the hassle.
                retval += ::obake::key_evaluate(k, si, ss) * ::obake::evaluate(c, sm);
            }
        }

        return retval;
    }
};

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
    requires series_default_evaluate_impl::algo<T, U> != 0 inline constexpr auto evaluate<T, U>
#else
inline constexpr auto evaluate<T, U, ::std::enable_if_t<series_default_evaluate_impl::algo<T, U> != 0>>
#endif
    = series_default_evaluate_impl{};

} // namespace customisation::internal

namespace customisation::internal
{

// Metaprogramming to establish the algorithm/return
// type of the default series trim computation.
template <typename T>
constexpr auto series_default_trim_algorithm_impl()
{
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, detail::type_c<void>{});

    if constexpr (!is_cvr_series_v<T>) {
        // Not a series type.
        return failure;
    } else {
        using rT = remove_cvref_t<T>;
        using cf_t = series_cf_t<rT>;
        using key_t = series_key_t<rT>;

        // We need to be able to:
        // - run key_trim_identify() on the keys,
        // - trim both cfs and keys,
        // all using const lvalue refs.
        if constexpr (::std::conjunction_v<is_trim_identifiable_key<const key_t &>, is_trimmable_key<const key_t &>,
                                           is_trimmable<const cf_t &>>) {
            // The return type is the original series type (after cvref removal),
            // because cf/key trimming is guaranteed not to change the cf/key
            // types.
            return ::std::make_pair(1, detail::type_c<rT>{});
        } else {
            return failure;
        }
    }
}

// Default implementation of series trim.
struct series_default_trim_impl {
    // A couple of handy shortcuts.
    template <typename T>
    static constexpr auto algo_ret = internal::series_default_trim_algorithm_impl<T>();

    template <typename T>
    static constexpr auto algo = series_default_trim_impl::algo_ret<T>.first;

    template <typename T>
    using ret_t = typename decltype(series_default_trim_impl::algo_ret<T>.second)::type;

    // Implementation.
    template <typename T>
    ret_t<T &&> operator()(T &&x_) const
    {
        // Sanity checks.
        static_assert(algo<T &&> != 0);
        static_assert(::std::is_same_v<ret_t<T &&>, remove_cvref_t<T>>);

        // Need only const access to x.
        const auto &x = ::std::as_const(x_);

        // Cache x's original symbol set.
        const auto &ss = x.get_symbol_set();

        // Run trim_identify() on all the keys.
        ::std::vector<int> trim_v(::obake::safe_cast<::std::vector<int>::size_type>(ss.size()), 1);
        for (const auto &t : x) {
            ::obake::key_trim_identify(trim_v, t.first, ss);
        }

        // Create the set of symbol indices for trimming,
        // and the trimmed symbol set.
        symbol_idx_set::sequence_type si_seq;
        si_seq.reserve(static_cast<decltype(si_seq.size())>(ss.size()));
        symbol_set::sequence_type new_ss_seq;
        new_ss_seq.reserve(static_cast<decltype(new_ss_seq.size())>(ss.size()));
        for (symbol_idx i = 0; i < ss.size(); ++i) {
            if (trim_v[i] != 0) {
                si_seq.push_back(i);
            } else {
                new_ss_seq.push_back(*ss.nth(i));
            }
        }
        symbol_idx_set si;
        si.adopt_sequence(::boost::container::ordered_unique_range_t{}, ::std::move(si_seq));
        symbol_set new_ss;
        new_ss.adopt_sequence(::boost::container::ordered_unique_range_t{}, ::std::move(new_ss_seq));

        // Prepare the return value.
        ret_t<T &&> retval;
        retval.set_symbol_set(new_ss);
        // NOTE: use the same number of segments as x
        // and reserve space for the same number of terms.
        retval.set_n_segments(retval.get_s_size());
        retval.reserve(x.size());

        for (const auto &t : x) {
            // NOTE: run all checks on insertion:
            // - we don't know if something becomes zero after
            //   trimming,
            // - we don't know if a key loses compatibility after
            //   trimming (this is difficult to impose as a runtime
            //   requirement on key_trim()),
            // - we don't know if keys are not unique any more after
            //   trimming (same problem as above),
            // - we don't know if we are going to go over the table
            //   size limit (as terms will be shuffled around after
            //   trimming).
            // We can always think about removing some checks at
            // a later stage.
            // NOTE: in the series_sym_extender() helper, we distinguish
            // the segmented vs non-segmented case. Perhaps we can do it
            // here as well in the future, if it makes sense for performance.
            retval.add_term(::obake::key_trim(t.first, si, ss), ::obake::trim(t.second));
        }

        return retval;
    }
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires(series_default_trim_impl::algo<T> != 0) inline constexpr auto trim<T>
#else
inline constexpr auto trim<T, ::std::enable_if_t<series_default_trim_impl::algo<T> != 0>>
#endif
    = series_default_trim_impl{};

} // namespace customisation::internal

namespace detail
{

// The type returned by the application of the functor
// F to a term of a series with key K, coefficient C and tag Tag.
// Everything done via const lvalue refs.
template <typename F, typename K, typename C, typename Tag>
using term_filter_return_t
    = decltype(::std::declval<const F &>()(::std::declval<const series_term_t<series<K, C, Tag>> &>()));

// NOTE: for now, pass the series with a const reference. In the future,
// we may want to allow for perfect forwarding to exploit move-construction
// of the coefficients, as done elsewhere (see rref cleaner).
template <typename K, typename C, typename Tag, typename F,
          ::std::enable_if_t<::std::is_convertible_v<detected_t<term_filter_return_t, F, K, C, Tag>, bool>, int> = 0>
inline series<K, C, Tag> filter_impl(const series<K, C, Tag> &s, const F &f)
{
    // Init the return value. Same symbol set
    // and same number of segments as s.
    series<K, C, Tag> retval;
    retval.set_symbol_set(s.get_symbol_set());
    retval.set_n_segments(s.get_s_size());

    // Do the filtering table by table.
    // NOTE: this can easily be parallelised.
    const auto n_tables = s._get_s_table().size();
    for (decltype(s._get_s_table().size()) table_idx = 0; table_idx < n_tables; ++table_idx) {
        // Fetch references to the input/output tables.
        const auto &in_table = s._get_s_table()[table_idx];
        auto &out_table = retval._get_s_table()[table_idx];

        for (const auto &t : in_table) {
            if (f(t)) {
                [[maybe_unused]] const auto res = out_table.insert(t);
                // The insertion must be successful,
                // as we are not changing the original keys.
                assert(res.second);
            }
        }
    }

    return retval;
}

} // namespace detail

#if defined(_MSC_VER)

struct filter_msvc {
    template <typename T, typename F>
    constexpr auto operator()(T &&s, const F &f) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::filter_impl(::std::forward<T>(s), f))
};

inline constexpr auto filter = filter_msvc{};

#else

// NOTE: do we need a concept/type trait for this? See also the testing.
// NOTE: force const reference passing for f as a hint
// that the implementation may be parallel.
inline constexpr auto filter =
    [](auto &&s, const auto &f) OBAKE_SS_FORWARD_LAMBDA(detail::filter_impl(::std::forward<decltype(s)>(s), f));

#endif

namespace detail
{

// NOTE: for now, pass the series with a const reference. In the future,
// we may want to allow for perfect forwarding to exploit rvalue
// semantics in series_sym_extender().
template <typename K, typename C, typename Tag, ::std::enable_if_t<is_symbols_mergeable_key_v<const K &>, int> = 0>
inline series<K, C, Tag> add_symbols_impl(const series<K, C, Tag> &s, const symbol_set &ss)
{
    const auto [merged_ss, ins_map, _] = detail::merge_symbol_sets(s.get_symbol_set(), ss);
    detail::ignore(_);

    if (ins_map.empty()) {
        // Empty insertion map: there are no
        // symbols to add.
        return s;
    }

    series<K, C, Tag> retval;
    // NOTE: the sym extender takes care of the segmentation/allocation,
    // it just needs the proper symbol set.
    retval.set_symbol_set(merged_ss);
    detail::series_sym_extender(retval, s, ins_map);

    return retval;
}

} // namespace detail

#if defined(_MSC_VER)

struct add_symbols_msvc {
    template <typename T>
    constexpr auto operator()(T &&s, const symbol_set &ss) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::add_symbols_impl(::std::forward<T>(s), ss))
};

inline constexpr auto add_symbols = add_symbols_msvc{};

#else

// NOTE: do we need a concept/type trait for this? See also the testing.
inline constexpr auto add_symbols = [](auto &&s, const symbol_set &ss)
    OBAKE_SS_FORWARD_LAMBDA(detail::add_symbols_impl(::std::forward<decltype(s)>(s), ss));

#endif

} // namespace obake

namespace boost::serialization
{

// Disable tracking for series.
template <typename K, typename C, typename Tag>
struct tracking_level<::obake::series<K, C, Tag>> : ::obake::detail::s11n_no_tracking<::obake::series<K, C, Tag>> {
};

} // namespace boost::serialization

#endif
