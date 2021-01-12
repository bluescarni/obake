// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_SERIES_HPP
#define OBAKE_SERIES_HPP

#include <algorithm>
#include <any>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <mutex>
#include <numeric>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/container/container_fwd.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>

#include <mp++/integer.hpp>

#include <obake/byte_size.hpp>
#include <obake/cf/cf_stream_insert.hpp>
#include <obake/cf/cf_tex_stream_insert.hpp>
#include <obake/config.hpp>
#include <obake/detail/abseil.hpp>
#include <obake/detail/fcast.hpp>
#include <obake/detail/ignore.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/safe_integral_arith.hpp>
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
using is_key = ::std::conjunction<is_semi_regular<T>, ::std::is_constructible<T, const symbol_set &>,
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
    is_in_place_addable<::std::add_lvalue_reference_t<T>, ::std::add_lvalue_reference_t<const T>>,
    is_in_place_addable<::std::add_lvalue_reference_t<T>, ::std::add_rvalue_reference_t<T>>,
    is_in_place_subtractable<::std::add_lvalue_reference_t<T>, ::std::add_lvalue_reference_t<const T>>,
    is_in_place_subtractable<::std::add_lvalue_reference_t<T>, ::std::add_rvalue_reference_t<T>>,
    is_negatable<::std::add_lvalue_reference_t<T>>>;

template <typename T>
inline constexpr bool is_cf_v = is_cf<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL Cf = is_cf_v<T>;

#endif

// A series tag must be a semi-regular class.
// NOTE: semi-regular already includes swappability.
template <typename T>
using is_series_tag = ::std::conjunction<::std::is_class<T>, is_semi_regular<T>>;

template <typename T>
inline constexpr bool is_series_tag_v = is_series_tag<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL series_tag = is_series_tag_v<T>;

#endif

// Forward declaration.
#if defined(OBAKE_HAVE_CONCEPTS)
template <Key, Cf, series_tag>
#else
template <typename K, typename C, typename Tag,
          typename = ::std::enable_if_t<::std::conjunction_v<is_key<K>, is_cf<C>, is_series_tag<Tag>>>>
#endif
class series;

namespace detail
{

template <typename T>
inline constexpr ::std::size_t series_rank_impl = 0;

template <typename K, typename C, typename Tag>
inline constexpr ::std::size_t series_rank_impl<series<K, C, Tag>> =
#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)
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
inline void series_add_term_table(S &s, Table &t, T &&key, Args &&...args)
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
inline void series_add_term(S &s, T &&key, Args &&...args)
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
            return ::std::is_constructible_v<C, T> ? 1 : 0;
        } else if constexpr (series_rank<rT> == series_rank<series_t>) {
            if constexpr (::std::conjunction_v<::std::is_same<series_key_t<rT>, K>,
                                               ::std::is_same<series_tag_t<rT>, Tag>>) {
                // Construction from equal rank, different coefficient type. Requires
                // to be able to construct C from the coefficient type of T.
                // The construction argument will be a const reference or an rvalue
                // reference, depending on whether T && is a mutable rvalue reference or not.
                // NOTE: we need to explicitly put T && here because this function
                // is invoked with a type T which was deduced from a forwarding
                // reference (T &&).
                using cf_conv_t = ::std::conditional_t<is_mutable_rvalue_reference_v<T &&>, series_cf_t<rT> &&,
                                                       const series_cf_t<rT> &>;
                return ::std::is_constructible_v<C, cf_conv_t> ? 2 : 0;
            } else {
                return 0;
            }
        } else {
            // Construction from higher rank. Requires that
            // series_t can be constructed from the coefficient
            // type of T.
            using series_conv_t = ::std::conditional_t<is_mutable_rvalue_reference_v<T &&>, series_cf_t<rT> &&,
                                                       const series_cf_t<rT> &>;
            return ::std::is_constructible_v<series_t, series_conv_t> ? 3 : 0;
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
using is_series_convertible = ::std::conjunction<::std::integral_constant<bool, series_rank<T> == 0u>,
                                                 ::std::is_object<T>, ::std::is_constructible<T, int>,
                                                 ::std::is_constructible<T, ::std::add_lvalue_reference_t<const C>>>;

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
    // NOTE: here we are duplicating a bit of internal
    // abseil code for integral hash mixing, with the intent
    // of avoiding the per-process seeding that abseil does.
    // See here for the original code:
    // https://github.com/abseil/abseil-cpp/blob/37dd2562ec830d547a1524bb306be313ac3f2556/absl/hash/internal/hash.h#L754
    // If/when abseil starts supporting DLL builds, we can
    // remove this code and switch back to using abseil's
    // own hash machinery for mixing.
    static constexpr ::std::uint64_t kMul
        = sizeof(::std::size_t) == 4u ? ::std::uint64_t{0xcc9e2d51ull} : ::std::uint64_t{0x9ddfea08eb382d69ull};
    ABSL_ATTRIBUTE_ALWAYS_INLINE static ::std::uint64_t Mix(::std::uint64_t state, ::std::uint64_t v)
    {
        using MultType = ::std::conditional_t<sizeof(::std::size_t) == 4u, ::std::uint64_t, ::absl::uint128>;
        // We do the addition in 64-bit space to make sure the 128-bit
        // multiplication is fast. If we were to do it as MultType the compiler has
        // to assume that the high word is non-zero and needs to perform 2
        // multiplications instead of one.
        MultType m = state + v;
        m *= kMul;
        return static_cast<::std::uint64_t>(m ^ (m >> (sizeof(m) * 8 / 2)));
    }
    template <typename K>
    ::std::size_t operator()(const K &k) const noexcept(noexcept(::obake::hash(k)))
    {
        // NOTE: mix with a compile-time seed.
        return static_cast<::std::size_t>(
            series_key_hasher::Mix(15124392053943080205ull, static_cast<::std::uint64_t>(::obake::hash(k))));
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
// conversion will take place. "to" is supposed to have the correct symbol set already,
// but, apart from that, it must be empty, and the number of segments and space
// reservation will be taken from "from".
// Another precondition is that to and from must be distinct objects.
template <typename To, typename From>
inline void series_sym_extender(To &to, From &&from, const symbol_idx_map<symbol_set> &ins_map)
{
    // NOTE: we assume that this helper is
    // invoked with a non-empty insertion map, and an empty
    // "to" series. "to" must have the correct symbol set.
    assert(!ins_map.empty());
    assert(to.empty());
    if constexpr (::std::is_same_v<remove_cvref_t<To>, remove_cvref_t<From>>) {
        assert(&to != &from);
    }

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

        for (const auto &[k, c] : from._get_s_table()[0]) {
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
template <Key K, Cf C, series_tag Tag>
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

    series()
        : m_s_table(1), m_log2_size(0),
          // NOTE: construct m_symbol_set cheaply from
          // a global object constructed from symbol_set{}.
          m_symbol_set(detail::ss_fw_default())
    {
    }
    series(const series &) = default;
    series(series &&other) noexcept
        : m_s_table(::std::move(other.m_s_table)), m_log2_size(::std::move(other.m_log2_size)),
          m_tag(::std::move(other.m_tag)), m_symbol_set(::std::move(other.m_symbol_set))
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
    // series from scalars - see for instance the default series pow()
    // implementation).
#if defined(OBAKE_HAVE_CONCEPTS)
    template <SeriesConstructible<K, C, Tag> T>
#else
    template <typename T, ::std::enable_if_t<is_series_constructible_v<T, K, C, Tag>, int> = 0>
#endif
    explicit series(T &&x) : series()
    {
        constexpr int algo = detail::series_generic_ctor_algorithm<T, K, C, Tag>;
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
                *this, m_s_table[0], K(m_symbol_set.get()), ::std::forward<T>(x));
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
            m_symbol_set = x.get_symbol_set_fw();

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
    // Constructor from generic lower-rank object
    // and symbol set.
    template <typename T, ::std::enable_if_t<detail::series_generic_ctor_algorithm<T, K, C, Tag> == 1, int> = 0>
    explicit series(T &&x, const symbol_set &s) : m_s_table(1), m_log2_size(0), m_symbol_set(s)
    {
        // NOTE: this is identical to the generic ctor code, not sure if it's worth it
        // to abstract it out in a separate function.
        // NOTE: disable key compat and table size checks: the key must be compatible
        // with the symbol set used for construction (key concept runtime requirement),
        // and we have only 1 table, so no size check needed. Also, the new term
        // will be unique by construction.
        detail::series_add_term_table<true, detail::sat_check_zero::on, detail::sat_check_compat_key::off,
                                      detail::sat_check_table_size::off, detail::sat_assume_unique::on>(
            *this, m_s_table[0], K(m_symbol_set.get()), ::std::forward<T>(x));
    }

    series &operator=(const series &) = default;
    series &operator=(series &&other) noexcept
    {
        // NOTE: assuming self-assignment is handled
        // correctly by the members.
        m_s_table = ::std::move(other.m_s_table);
        m_log2_size = ::std::move(other.m_log2_size);
        m_tag = ::std::move(other.m_tag);
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
    template <typename T, ::std::enable_if_t<is_series_constructible_v<T, K, C, Tag>, int> = 0>
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
                // An empty series is a single-coefficient series
                // equivalent to zero.
                return T(0);
            case 1u: {
                // A single-term series is single-coefficient if the
                // only key is unitary.
                const auto it = cbegin();
                if (obake_likely(::obake::key_is_one(it->first, m_symbol_set.get()))) {
                    return T(it->second);
                }
                [[fallthrough]];
            }
            default:
                // Multiple terms in the series, or we fell through
                // from the previous case: cannot be a
                // single-coefficient series.
                obake_throw(::std::invalid_argument,
                            "Cannot convert a series of type '" + ::obake::type_name<series>()
                                + "' to on object of type '" + ::obake::type_name<T>()
                                + "', because the series does not consist of a single coefficient");
        }
    }

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
                assert(!::obake::key_is_zero(k, m_symbol_set.get()) && !::obake::is_zero(c));
                // No incompatible keys.
                assert(::obake::key_is_compatible(k, m_symbol_set.get()));
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

        if (m_s_table.size() > 1u) {
            // Clear the tables in parallel if there's more than 1.
            ::tbb::parallel_for(::tbb::blocked_range(m_s_table.begin(), m_s_table.end()), [](const auto &range) {
                for (auto &t : range) {
                    // NOTE: move assigning a new empty table
                    // should ensure that the memory in t
                    // is deallocated.
                    t = table_type{};
                }
            });
        }
    }

    // Member function implementation of the swap primitive.
    void swap(series &other) noexcept
    {
        using ::std::swap;

        swap(m_s_table, other.m_s_table);
        swap(m_log2_size, other.m_log2_size);
        swap(m_tag, other.m_tag);
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
    // documented.
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
                return ::obake::key_is_one(cbegin()->first, m_symbol_set.get());
            default:
                return false;
        }
    }

private:
    // A small helper to select the (const) iterator of table_type, depending on whether
    // T is const or not. Used in the iterator implementation below.
    template <typename T>
    using local_it_t = ::std::conditional_t<::std::is_const_v<T>, typename table_type::const_iterator,
                                            typename table_type::iterator>;

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
        // Default constructor.
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

        // Default the copy/move ctors/assignment operators.
        iterator_impl(const iterator_impl &) = default;
        iterator_impl(iterator_impl &&) = default;
        iterator_impl &operator=(const iterator_impl &) = default;
        iterator_impl &operator=(iterator_impl &&) = default;

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
        // set to the size of the segmented table
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
        return m_symbol_set.get();
    }
    void set_symbol_set(const symbol_set &s)
    {
        if (obake_unlikely(!empty())) {
            obake_throw(::std::invalid_argument, "A symbol set can be set only in an empty series, but this series has "
                                                     + detail::to_string(size()) + " terms");
        }

        m_symbol_set = s;
    }

    const detail::ss_fw &get_symbol_set_fw() const
    {
        return m_symbol_set;
    }
    void set_symbol_set_fw(const detail::ss_fw &s)
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

    // NOTE: this method requires that the term
    // being inserted is not from this series.
    template <bool Sign = true,
#if defined(OBAKE_HAVE_CONCEPTS)
              SameCvr<K> T, typename... Args>
    requires Constructible<C, Args...>
#else
              typename T, typename... Args,
              ::std::enable_if_t<::std::conjunction_v<is_same_cvr<T, K>, ::std::is_constructible<C, Args...>>, int> = 0>
#endif
        void add_term(T &&key, Args &&...args)
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
    // The number of segments, the symbol set and
    // the tag will be kept intact.
    void clear_terms() noexcept
    {
        for (auto &t : m_s_table) {
            t.clear();
        }
    }

    // Clear the series.
    // This will remove all the terms and symbols.
    // The number of segments will be kept intact.
    // The tag will be reset to a def-constructed value.
    void clear() noexcept
    {
        clear_terms();

        m_tag = Tag{};

        // NOTE: cheap assignment via a ss_fw
        // constructed from a symbol_set{}.
        m_symbol_set = detail::ss_fw_default();
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

    // Return a bunch of statistics
    // about the hash table(s) in string
    // format.
    ::std::string table_stats() const
    {
        ::std::ostringstream oss;

        const auto s = size();
        const auto ntables = m_s_table.size();

        oss << "Total number of terms             : " << s << '\n';
        oss << "Total number of tables            : " << ntables << '\n';

        if (s != 0u) {
            oss << "Average terms per table           : " << static_cast<double>(s) / static_cast<double>(ntables)
                << '\n';
            const auto [it_min, it_max] = ::std::minmax_element(
                m_s_table.cbegin(), m_s_table.cend(),
                [](const table_type &t1, const table_type &t2) { return t1.size() < t2.size(); });
            oss << "Min/max terms per table           : " << it_min->size() << '/' << it_max->size() << '\n';
        }

        oss << "Total size in bytes               : " << ::obake::byte_size(*this) << '\n';

        return oss.str();
    }

    // Tag access.
    Tag &tag()
    {
        return m_tag;
    }
    const Tag &base() const
    {
        return m_tag;
    }

private:
    // Serialisation.
    template <class Archive>
    void save(Archive &ar, unsigned) const
    {
        ar << m_log2_size;
        ar << m_tag;
        ar << m_symbol_set;

        for (const auto &tab : m_s_table) {
            ar << tab.size();

            // Save separately key and coefficient.
            for (const auto &[k, c] : tab) {
                ar << k;
                ar << c;
            }
        }
    }
    template <class Archive>
    void load(Archive &ar, unsigned)
    {
        // Empty out before doing anything.
        clear();

        try {
            // Recover the segmented size.
            unsigned log2_size;
            ar >> log2_size;
            set_n_segments(log2_size);

            // Recover the tag.
            ar >> m_tag;

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
            // LCOV_EXCL_START
        } catch (...) {
            // Avoid inconsistent state in case of exceptions.
            clear();
            throw;
        }
        // LCOV_EXCL_STOP
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

private:
    s_table_type m_s_table;
    unsigned m_log2_size;
    Tag m_tag;
    detail::ss_fw m_symbol_set;
};

// Free function implementation of the swapping primitive.
template <typename K, typename C, typename Tag>
inline void swap(series<K, C, Tag> &s1, series<K, C, Tag> &s2) noexcept
{
    s1.swap(s2);
}

namespace customisation::internal
{

// Helper to compare series with identical symbol sets.
// Two series are considered equal if they have the same
// number of terms and if for each term in one series
// an equal term exists in the other series (term equality
// is tested by comparing both the coefficient and the key).
// NOTE: in this helper, T and U may have different coefficient
// types, but rank, key and tag are the same.
template <typename T, typename U>
inline bool series_cmp_identical_ss(const T &lhs, const U &rhs)
{
    assert(lhs.get_symbol_set() == rhs.get_symbol_set());

    static_assert(::std::is_same_v<series_tag_t<T>, series_tag_t<U>>);

    // Compare the tags, if possible.
    if constexpr (is_equality_comparable_v<const series_tag_t<T> &>) {
        if (lhs.tag() != rhs.tag()) {
            return false;
        }
    }

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
    for (const auto &[k, c] : lhs) {
        const auto it = rhs.find(k);
        if (it == rhs_end || c != it->second) {
            return false;
        }
    }

    return true;
}

// Helper to determine if two series of the same type are identical.
// They are if the symbol sets are equal and if the series
// compare equal according to series_cmp_identical_ss().
template <typename S>
inline bool series_are_identical(const S &s1, const S &s2)
{
    return s1.get_symbol_set() == s2.get_symbol_set() && internal::series_cmp_identical_ss(s1, s2);
}

// The series pow cache for a specific series type.
// It maps a series instance x to a vector of natural
// powers of x. Everything is type-erased as we need
// to store instances of this type as elements of another map.
// NOTE: the key in this map is the base, the vectors
// are the base raised to its natural powers. All these
// std::any will contain instances of the base type.
using series_te_pow_map_t = ::std::unordered_map<::std::any, ::std::vector<::std::any>,
                                                 // Hashing functor.
                                                 ::std::function<::std::size_t(const ::std::any &)>,
                                                 // Equality comparator.
                                                 ::std::function<bool(const ::std::any &, const ::std::any &)>>;

// Series pow cache. It maps a C++ series type, represented
// as a type_index, to a series_te_pow_map_t map.
using series_pow_map_t = ::std::unordered_map<::std::type_index, series_te_pow_map_t>;

// Function to fetch the global series pow cache and associated mutex.
OBAKE_DLL_PUBLIC ::std::tuple<series_pow_map_t &, ::std::mutex &> get_series_pow_map();

// Function to clear the global series pow cache.
OBAKE_DLL_PUBLIC void clear_series_pow_map();

// Fetch the n-th natural power of the input
// series 'base' from the global cache. If the
// power is not present in the cache already,
// it will be computed on the fly.
template <typename Base>
inline Base series_pow_from_cache(const Base &base, unsigned n)
{
    // Fetch the global data.
    auto [map, mutex] = internal::get_series_pow_map();

    // Turn the type into a type_index.
    ::std::type_index t_idx(typeid(Base));

    // The concrete hasher and equality comparator
    // functors for use in series_te_pow_map_t.
    // They will be wrapped in a std::function.
    struct hasher {
        ::std::size_t operator()(const ::std::any &x) const
        {
            const auto &bcast = ::std::any_cast<const Base &>(x);

            // Init retval with the hash of the tag, if available,
            // zero otherwise.
            auto retval = [&bcast]() -> ::std::size_t {
                if constexpr (is_hashable_v<const series_tag_t<Base> &>) {
                    return ::obake::hash(bcast.tag());
                } else {
                    detail::ignore(bcast);
                    return 0;
                }
            }();

            // Combine the hashes of all terms
            // via addition, so that their order
            // does not matter.
            for (const auto &t : bcast) {
                // NOTE: use the same hasher used in the implementation
                // of series.
                // NOTE: parallelisation opportunities here for
                // segmented tables.
                retval += detail::series_key_hasher{}(t.first);
            }

            return retval;
        }
    };

    struct comparer {
        bool operator()(const ::std::any &x, const ::std::any &y) const
        {
            // NOTE: need to use series_are_identical() (and not the comparison operator)
            // because the comparison operator does symbol merging, and thus it is
            // not consistent with the hasher defined above (i.e., two series may
            // compare equal according to operator==() and have different hashes).
            // NOTE: with these choices of hasher/comparer, the requirement that
            // cmp(a, b) == true -> hash(a) == hash(b) is always satisfied (even if, say,
            // the user customises series_equal_to()).
            return internal::series_are_identical(::std::any_cast<const Base &>(x), ::std::any_cast<const Base &>(y));
        }
    };

    // Lock down before accessing the cache.
    ::std::lock_guard<::std::mutex> lock(mutex);

    // Try first to locate the series_te_pow_map_t for the current type,
    // then, in that map, try to locate 'base'. Use try_emplace() so that
    // table elements are created as needed.
    auto it = map.try_emplace(t_idx, 0, hasher{}, comparer{}).first->second.try_emplace(base).first;

    // Fetch a reference to the base and the corresponding
    // exponentiation vector.
    const auto &b = ::std::any_cast<const Base &>(it->first);
    auto &v = it->second;

    // If the exponentiation vector is empty, init it with
    // base**0 = 1.
    if (v.empty()) {
        // NOTE: constructability from 1 is ensured by the
        // constructability of the return coefficient type from int
        // (and the return type is guaranteed to be the same as
        // the Base type in this function).
        v.emplace_back(Base(1));
    }

    // Fill in the missing powers as needed.
    while (v.size() <= n) {
        v.emplace_back(::std::any_cast<const Base &>(v.back()) * b);
    }

    // Return a copy of the desired power.
    // NOTE: returnability is guaranteed because
    // the return type is a series.
    return ::std::any_cast<const Base &>(v[static_cast<decltype(v.size())>(n)]);
}

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

template <typename T, typename U>
inline constexpr auto series_default_pow_algorithm = internal::series_default_pow_algorithm_impl<T, U>();

template <typename T, typename U>
inline constexpr int series_default_pow_algo = series_default_pow_algorithm<T, U>.first;

template <typename T, typename U>
using series_default_pow_ret_t = typename decltype(series_default_pow_algorithm<T, U>.second)::type;

// Default implementation of series exponentiation.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires(series_default_pow_algo<T &&, U &&> != 0)
#else
template <typename T, typename U, ::std::enable_if_t<series_default_pow_algo<T &&, U &&> != 0, int> = 0>
#endif
    inline series_default_pow_ret_t<T &&, U &&> pow(pow_t, T &&b, U &&e_)
{
    using ret_t = series_default_pow_ret_t<T &&, U &&>;

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
            return ret_t(::obake::pow(zero, e));
        } else {
            // Return the only coefficient raised to the exponent.
            return ret_t(::obake::pow(b.cbegin()->second, e));
        }
    }

    // Handle the case of zero exponent: anything to the power
    // of zero is 1.
    if (::obake::is_zero(e)) {
        // NOTE: construct directly from 1: 1 is rank 0,
        // construction then forwards 1 to the construction
        // of an internal coefficient.
        return ret_t(1);
    }

    // Let's determine if we can run exponentiation
    // by repeated multiplications, backed by a cache:
    // - e must be safely-convertible to unsigned,
    // - T * T must be defined and return T (via const lvalue
    //   references),
    // - T must be the same as ret_t,
    // - the coefficient of T must be
    //   equality-comparable (for the pow cache to work).
    // NOTE: the idea here is that, for ease of reasoning and
    // implementation, we want the base type, its product type
    // and the return type to be all the same.
    if constexpr (::std::conjunction_v<is_safely_convertible<const rU &, unsigned &>,
                                       // NOTE: this also checks that mul_t is defined, as
                                       // nonesuch is not a series type.
                                       ::std::is_same<rT, detected_t<detail::mul_t, const rT &, const rT &>>,
                                       ::std::is_same<rT, ret_t>, is_equality_comparable<const series_cf_t<rT> &>>) {
        unsigned un;
        if (obake_unlikely(!::obake::safe_convert(un, e))) {
            if constexpr (is_stream_insertable_v<const rU &>) {
                // Provide better error message if U is ostreamable.
                ::std::ostringstream oss;
                static_cast<::std::ostream &>(oss) << e;
                obake_throw(::std::invalid_argument, "Invalid exponent for series exponentiation via repeated "
                                                     "multiplications: the exponent ("
                                                         + oss.str()
                                                         + ") cannot be converted into a non-negative integral value");
            } else {
                obake_throw(::std::invalid_argument,
                            "Invalid exponent for series exponentiation via repeated "
                            "multiplications: the exponent cannot be converted into a non-negative integral value");
            }
        }

        return internal::series_pow_from_cache(b, un);
    } else {
        obake_throw(::std::invalid_argument,
                    "Cannot compute the power of a series of type '" + ::obake::type_name<rT>()
                        + "': the series does not consist of a single coefficient, "
                          "and exponentiation via repeated multiplications is not possible (either because the "
                          "exponent cannot be converted to a non-negative integral value, or because the "
                          "series/coefficient types do not support the necessary operations)");
    }
}

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

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T>
requires CvrSeries<T> && (!::std::is_const_v<::std::remove_reference_t<T>>)
#else
template <typename T,
          ::std::enable_if_t<
              ::std::conjunction_v<is_cvr_series<T>, ::std::negation<::std::is_const<::std::remove_reference_t<T>>>>,
              int> = 0>
#endif
inline void negate(negate_t, T &&x)
{
    detail::series_default_negate_impl(::std::forward<T>(x));
}

} // namespace customisation::internal

// Customise obake::is_zero() for series types.
namespace customisation::internal
{

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T>
requires CvrSeries<T>
#else
template <typename T, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
#endif
    inline bool is_zero(is_zero_t, const T &x)
{
    return x.empty();
}

} // namespace customisation::internal

// Customise obake::byte_size() for series types.
namespace customisation::internal
{

struct series_default_byte_size_impl {
    // Helper to compute the byte size of a single table.
    template <typename T, typename Tab>
    static auto st_byte_size(const Tab &tab)
    {
        ::std::size_t ret = 0;

        // Accumulate the byte size for all terms in the table
        for (const auto &[k, c] : tab) {
            // NOTE: account for possible padding in the series term class.
            static_assert(sizeof(k) + sizeof(c) <= sizeof(series_term_t<T>));
            ret += ::obake::byte_size(k) + ::obake::byte_size(c) + (sizeof(series_term_t<T>) - (sizeof(k) + sizeof(c)));
        }

        // Add the space occupied by the unused slots.
        assert(tab.capacity() >= tab.size());
        ret += (tab.capacity() - tab.size()) * sizeof(series_term_t<T>);

        return ret;
    }
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

        if (x._get_s_table().size() > 1u) {
            retval += ::tbb::parallel_reduce(
                ::tbb::blocked_range(x._get_s_table().begin(), x._get_s_table().end()), ::std::size_t(0),
                [](const auto &r, ::std::size_t init) {
                    for (const auto &tab : r) {
                        init += series_default_byte_size_impl::st_byte_size<T>(tab);
                    }

                    return init;
                },
                [](auto n1, auto n2) { return n1 + n2; });
        } else {
            for (const auto &tab : x._get_s_table()) {
                retval += series_default_byte_size_impl::st_byte_size<T>(tab);
            }
        }

        // Finally, add the contribution from the tag, if available.
        if constexpr (is_size_measurable_v<const series_tag_t<T> &>) {
            retval += ::obake::byte_size(x.base());
            // NOTE: because the sizeof() of the tag was counted
            // in sizeof(T), remove it from the total because
            // it is (presumably) being counted in the tag's byte_size()
            // implementation.
            retval -= sizeof(series_tag_t<T>);
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

    // Stream out the tag, if supported.
    if constexpr (is_stream_insertable_v<const series_tag_t<remove_cvref_t<T>> &>) {
        os << ::std::as_const(s).tag() << '\n';
    }

    series_stream_terms_impl<false>(os, s);
}

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

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
            if constexpr (::std::conjunction_v<::std::is_constructible<ret_t, U>,
                                               ::std::is_constructible<ret_cf_t, T>>) {
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
            if constexpr (::std::conjunction_v<::std::is_constructible<ret_t, T>,
                                               ::std::is_constructible<ret_cf_t, U>>) {
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
                              ::std::is_constructible<ret_t, T>, ::std::is_constructible<ret_t, U>,
                              // We may need to copy/move convert the original coefficients
                              // to ret_cf_t.
                              ::std::is_constructible<ret_cf_t, cf1_t>, ::std::is_constructible<ret_cf_t, cf2_t>,
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
        // Both T and U are series, same rank, possibly different cf (but same key).

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

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

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

namespace customisation
{

// External customisation point for obake::series_in_place_add().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto series_in_place_add = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto series_in_place_add_impl(T &&x, U &&y, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::series_in_place_add<T &&, U &&>)(::std::forward<T>(x),
                                                                               ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto series_in_place_add_impl(T &&x, U &&y, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(series_in_place_add(::std::forward<T>(x), ::std::forward<U>(y)));

// Meta-programming to establish the algorithm and return type
// of the default implementation of series in-place add/sub. It will return
// a pair containing an integral value (1, 2 or 3) signalling the algorithm
// to be used in the implementation, and a type_c wrapper representing
// the return type of the operation. If the add/sub implementation is not
// well-defined for the input types, it will return (0, void).
template <bool Sign, typename T, typename U>
constexpr auto series_default_in_place_addsub_algorithm_impl()
{
    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    constexpr auto rank_T = series_rank<rT>;
    [[maybe_unused]] constexpr auto rank_U = series_rank<rU>;

    // Shortcut for signalling that the add/sub implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, type_c<void>{});

    if constexpr (!rank_T) {
        // The lhs is not a series, return failure. This will disable
        // the default series_in_place_add/sub() implementations.
        return failure;
    } else if constexpr (rank_T < rank_U) {
        // The rank of T is less than the rank of U.
        // We will be delegating to the binary operator.
        if constexpr (Sign) {
            if constexpr (::std::conjunction_v<
                              is_detected<add_t, T, U>,
                              ::std::is_assignable<::std::remove_reference_t<T> &, detected_t<add_t, T, U>>>) {
                // NOTE: use rT & here because in the implementation
                // below we return the value returned by a series
                // assignment operator, which is always rT &.
                return ::std::make_pair(1, type_c<rT &>{});
            } else {
                return failure;
            }
        } else {
            if constexpr (::std::conjunction_v<
                              is_detected<sub_t, T, U>,
                              ::std::is_assignable<::std::remove_reference_t<T> &, detected_t<sub_t, T, U>>>) {
                return ::std::make_pair(1, type_c<rT &>{});
            } else {
                return failure;
            }
        }
    } else if constexpr (rank_T > rank_U) {
        // The rank of T is larger than the rank of U.
        // Implement via direct insertion: T must not be
        // const (after reference removal), and the cf
        // type of T must be constructible from U.
        if constexpr (::std::conjunction_v<::std::negation<::std::is_const<::std::remove_reference_t<T>>>,
                                           ::std::is_constructible<series_cf_t<rT>, U>>) {
            // NOTE: we can use rT & because in the implementation
            // we return an lvalue of something which is not
            // const (as ensured by the condition in the if
            // statement above).
            return ::std::make_pair(2, type_c<rT &>{});
        } else {
            return failure;
        }
    } else {
        // Equal ranks:
        // - key/tag must match,
        // - T must not be const (need to insert into it),
        // - the coefficient type of T must be constructible
        //   from the coefficient type of U (both const lvalue
        //   and rvalue variants, because depending on symbol
        //   merging we may have a runtime choice between
        //   rvalue and lvalue regardless of the original
        //   type of U),
        // - the key type must be symbols mergeable.
        if constexpr (::std::conjunction_v<::std::is_same<series_key_t<rT>, series_key_t<rU>>,
                                           ::std::is_same<series_tag_t<rT>, series_tag_t<rU>>,
                                           ::std::negation<::std::is_const<::std::remove_reference_t<T>>>,
                                           ::std::is_constructible<series_cf_t<rT>, series_cf_t<rU> &&>,
                                           ::std::is_constructible<series_cf_t<rT>, const series_cf_t<rU> &>,
                                           // We may need to merge new symbols into the original key type.
                                           // NOTE: the key types of T and U must be identical at the moment,
                                           // so checking only T's key type is enough.
                                           // NOTE: the merging is done via a const ref.
                                           is_symbols_mergeable_key<const series_key_t<rT> &>>) {
            // NOTE: we can use rT & because in the implementation
            // we return an lvalue of something which is not
            // const (as ensured by the condition in the if
            // statement above).
            return ::std::make_pair(3, type_c<rT &>{});
        } else {
            return failure;
        }
    }
}

// Shortcuts.
template <bool Sign, typename T, typename U>
inline constexpr auto series_default_in_place_addsub_algorithm
    = detail::series_default_in_place_addsub_algorithm_impl<Sign, T, U>();

template <bool Sign, typename T, typename U>
using series_default_in_place_addsub_ret_t =
    typename decltype(series_default_in_place_addsub_algorithm<Sign, T, U>.second)::type;

// Default implementation of the in-place add/sub primitive for series.
template <bool Sign, typename T, typename U>
inline series_default_in_place_addsub_ret_t<Sign, T &&, U &&> series_default_in_place_addsub_impl(T &&x, U &&y)
{
    using rT [[maybe_unused]] = remove_cvref_t<T>;
    using rU [[maybe_unused]] = remove_cvref_t<U>;

    // Determine the algorithm.
    constexpr int algo = series_default_in_place_addsub_algorithm<Sign, T &&, U &&>.first;
    static_assert(algo > 0 && algo <= 3);

    if constexpr (algo == 1) {
        // The rank of T is less than the rank of U.
        // Delegate to the binary operator.
        if constexpr (Sign) {
            return x = ::std::forward<T>(x) + ::std::forward<U>(y);
        } else {
            return x = ::std::forward<T>(x) - ::std::forward<U>(y);
        }
    } else if constexpr (algo == 2) {
        // The rank of U is less than the rank of T.
        // NOTE: we can turn off key compat check, as we know
        // the new key will be compatible by construction. The other
        // checks are needed. Also, because we don't know
        // the segmentation of y, we need to use series_add_term()
        // instead of series_add_term_table().
        detail::series_add_term<Sign, sat_check_zero::on, sat_check_compat_key::off, sat_check_table_size::on,
                                sat_assume_unique::off>(x, series_key_t<rT>(x.get_symbol_set()), ::std::forward<U>(y));

        return x;
    } else {
        // Both T and U are series, same rank, possibly different cf (but same key).

        // Implementation for identical symbol sets.
        auto in_place_with_identical_ss = [](auto &lhs, auto &&rhs) {
            assert(lhs.get_symbol_set() == rhs.get_symbol_set());

            // We may end up moving coefficients from rhs.
            // Make sure we will clear it out properly.
            using rhs_t = decltype(rhs);
            series_rref_clearer<rhs_t> rhs_c(::std::forward<rhs_t>(rhs));

            // Distinguish the two cases in which the lhs table
            // is segmented or not.
            if (lhs._get_s_table().size() > 1u) {
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
                                                sat_check_table_size::on, sat_assume_unique::off>(lhs, k,
                                                                                                  ::std::move(c));
                    } else {
                        detail::series_add_term<Sign, sat_check_zero::on, sat_check_compat_key::off,
                                                sat_check_table_size::on, sat_assume_unique::off>(lhs, k,
                                                                                                  ::std::as_const(c));
                    }
                }
            } else {
                assert(lhs._get_s_table().size() == 1u);

                auto &t = lhs._get_s_table()[0];

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
                            lhs, t, k, ::std::move(c));
                    } else {
                        detail::series_add_term_table<Sign, sat_check_zero::on, sat_check_compat_key::off,
                                                      sat_check_table_size::off, sat_assume_unique::off>(
                            lhs, t, k, ::std::as_const(c));
                    }
                }
            }
        };

        if (x.get_symbol_set() == y.get_symbol_set()) {
            // Same symbol sets, run the implementation
            // directly on x and y.
            if constexpr (::std::is_same_v<rT, rU>) {
                // NOTE: if x and y are of the same type,
                // and they are the same object, use a copy
                // of y *without forwarding*, otherwise
                // we will be ending up possibly erasing
                // or re-inserting terms from a series into
                // itself above.
                // NOTE: to improve performance, we could
                // implement addition as a scalar multiplication
                // by 2, and subtraction as a clear()ing of
                // x. Keep it in mind for the future.
                if (&x == &y) {
                    in_place_with_identical_ss(x, rU(y));
                } else {
                    in_place_with_identical_ss(x, ::std::forward<U>(y));
                }
            } else {
                in_place_with_identical_ss(x, ::std::forward<U>(y));
            }
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
                case 0u: {
                    // Both x and y need to be extended.
                    rT a;
                    rU b;
                    a.set_symbol_set(merged_ss);
                    b.set_symbol_set(merged_ss);
                    detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);
                    detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);
                    x = ::std::move(a);

                    in_place_with_identical_ss(x, ::std::move(b));
                    break;
                }
                case 1u: {
                    // x already has the correct symbol
                    // set, extend only y.
                    rU b;
                    b.set_symbol_set(merged_ss);
                    detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

                    in_place_with_identical_ss(x, ::std::move(b));
                    break;
                }
                case 2u: {
                    // y already has the correct symbol
                    // set, extend only x.
                    rT a;
                    a.set_symbol_set(merged_ss);
                    detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);
                    x = ::std::move(a);

                    in_place_with_identical_ss(x, ::std::forward<U>(y));
                    break;
                }
            }
        }

        return x;
    }
}

template <typename T, typename U>
inline constexpr int series_default_in_place_add_algo = series_default_in_place_addsub_algorithm<true, T, U>.first;

// Lowest priority: the default implementation for series.
template <typename T, typename U, ::std::enable_if_t<series_default_in_place_add_algo<T &&, U &&> != 0, int> = 0>
constexpr auto series_in_place_add_impl(T &&x, U &&y, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(detail::series_default_in_place_addsub_impl<true>(::std::forward<T>(x),
                                                                                ::std::forward<U>(y)));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct series_in_place_add_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(static_cast<::std::add_lvalue_reference_t<remove_cvref_t<T>>>(
            detail::series_in_place_add_impl(::std::forward<T>(x), ::std::forward<U>(y), detail::priority_tag<2>{})))
};

inline constexpr auto series_in_place_add = series_in_place_add_msvc{};

#else

// NOTE: explicitly cast the result of the implementation
// to an lvalue reference to the type of x, so that
// we disable the implementation if such conversion is
// malformed. The idea is that we want an implementation
// which feels like the builtin operators.
// NOTE: the default implementation does not support
// move-adding in-place a coefficient belonging to a series
// to the same series, as that may result in the original
// series coefficient becoming zero after being moved-from.
// This is a corner case, but perhaps we need to document it?
inline constexpr auto series_in_place_add = [](auto &&x, auto &&y) OBAKE_SS_FORWARD_LAMBDA(
    static_cast<::std::add_lvalue_reference_t<remove_cvref_t<decltype(x)>>>(detail::series_in_place_add_impl(
        ::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<2>{})));

#endif

// NOTE: handle separately the two cases
// series lhs vs non-series lhs.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T>
#else
template <typename T, typename U, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
#endif
    constexpr auto operator+=(T &&x, U &&y)
        OBAKE_SS_FORWARD_FUNCTION(::obake::series_in_place_add(::std::forward<T>(x), ::std::forward<U>(y)));

// NOTE: if the lhs is not a series, just implement
// on top of the binary operator.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
    requires(!CvrSeries<T>)
    && CvrSeries<U>
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<::std::negation<is_cvr_series<T>>, is_cvr_series<U>>, int> = 0>
#endif
    constexpr auto operator+=(T &&x, U &&y)
        OBAKE_SS_FORWARD_FUNCTION(x = static_cast<remove_cvref_t<T>>(::std::forward<T>(x) + ::std::forward<U>(y)));

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

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

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

namespace customisation
{

// External customisation point for obake::series_in_place_sub().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto series_in_place_sub = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto series_in_place_sub_impl(T &&x, U &&y, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::series_in_place_sub<T &&, U &&>)(::std::forward<T>(x),
                                                                               ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto series_in_place_sub_impl(T &&x, U &&y, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(series_in_place_sub(::std::forward<T>(x), ::std::forward<U>(y)));

template <typename T, typename U>
inline constexpr int series_default_in_place_sub_algo = series_default_in_place_addsub_algorithm<false, T, U>.first;

// Lowest priority: the default implementation for series.
template <typename T, typename U, ::std::enable_if_t<series_default_in_place_sub_algo<T &&, U &&> != 0, int> = 0>
constexpr auto series_in_place_sub_impl(T &&x, U &&y, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(detail::series_default_in_place_addsub_impl<false>(::std::forward<T>(x),
                                                                                 ::std::forward<U>(y)));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct series_in_place_sub_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(static_cast<::std::add_lvalue_reference_t<remove_cvref_t<T>>>(
            detail::series_in_place_sub_impl(::std::forward<T>(x), ::std::forward<U>(y), detail::priority_tag<2>{})))
};

inline constexpr auto series_in_place_sub = series_in_place_sub_msvc{};

#else

// NOTE: explicitly cast the result of the implementation
// to an lvalue reference to the type of x, so that
// we disable the implementation if such conversion is
// malformed. The idea is that we want an implementation
// which feels like the builtin operators.
// NOTE: the default implementation does not support
// move-subtracting in-place a coefficient belonging to a series
// from the same series, as that may result in the original
// series coefficient becoming zero after being moved-from.
// This is a corner case, but perhaps we need to document it?
inline constexpr auto series_in_place_sub = [](auto &&x, auto &&y) OBAKE_SS_FORWARD_LAMBDA(
    static_cast<::std::add_lvalue_reference_t<remove_cvref_t<decltype(x)>>>(detail::series_in_place_sub_impl(
        ::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<2>{})));

#endif

// NOTE: handle separately the two cases
// series lhs vs non-series lhs.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrSeries<T>
#else
template <typename T, typename U, ::std::enable_if_t<is_cvr_series_v<T>, int> = 0>
#endif
    constexpr auto operator-=(T &&x, U &&y)
        OBAKE_SS_FORWARD_FUNCTION(::obake::series_in_place_sub(::std::forward<T>(x), ::std::forward<U>(y)));

// NOTE: if the lhs is not a series, just implement
// on top of the binary operator.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
    requires(!CvrSeries<T>)
    && CvrSeries<U>
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<::std::negation<is_cvr_series<T>>, is_cvr_series<U>>, int> = 0>
#endif
    constexpr auto operator-=(T &&x, U &&y)
        OBAKE_SS_FORWARD_FUNCTION(x = static_cast<remove_cvref_t<T>>(::std::forward<T>(x) - ::std::forward<U>(y)));

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
                                  ::std::is_constructible<ret_t, U>,
                                  is_in_place_multipliable<ret_cf_t &, ::std::add_lvalue_reference_t<const rT>>>) {
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
                                  ::std::is_constructible<ret_t, T>,
                                  is_in_place_multipliable<ret_cf_t &, ::std::add_lvalue_reference_t<const rU>>>) {
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

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

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
    constexpr auto operator*=(T &&x, U &&y) OBAKE_SS_FORWARD_FUNCTION(x = ::std::forward<T>(x) * ::std::forward<U>(y));

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
    requires(!CvrSeries<T>)
    && CvrSeries<U>
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<::std::negation<is_cvr_series<T>>, is_cvr_series<U>>, int> = 0>
#endif
    constexpr auto operator*=(T &&x, U &&y)
        OBAKE_SS_FORWARD_FUNCTION(x = static_cast<remove_cvref_t<T>>(::std::forward<T>(x) * ::std::forward<U>(y)));

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
                              ::std::is_constructible<ret_t, T>,
                              is_in_place_divisible<ret_cf_t &, ::std::add_lvalue_reference_t<const rU>>>) {
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

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

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
    constexpr auto operator/=(T &&x, U &&y) OBAKE_SS_FORWARD_FUNCTION(x = ::std::forward<T>(x) / ::std::forward<U>(y));

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
    requires(!CvrSeries<T>)
    && CvrSeries<U>
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<::std::negation<is_cvr_series<T>>, is_cvr_series<U>>, int> = 0>
#endif
    constexpr auto operator/=(T &&x, U &&y)
        OBAKE_SS_FORWARD_FUNCTION(x = static_cast<remove_cvref_t<T>>(::std::forward<T>(x) / ::std::forward<U>(y)));

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
        if (x.get_symbol_set() == y.get_symbol_set()) {
            return customisation::internal::series_cmp_identical_ss(x, y);
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

                    return customisation::internal::series_cmp_identical_ss(x, b);
                }
                case 2u: {
                    // y already has the correct symbol
                    // set, extend only x.
                    rT a;
                    a.set_symbol_set(merged_ss);
                    detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);

                    return customisation::internal::series_cmp_identical_ss(a, y);
                }
            }

            // Both x and y need to be extended.
            rT a;
            rU b;
            a.set_symbol_set(merged_ss);
            b.set_symbol_set(merged_ss);
            detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);
            detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

            return customisation::internal::series_cmp_identical_ss(a, b);
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

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

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

// Common requirements for the degree type
// in the default series degree computation.
template <typename DegreeType>
using series_default_degree_type_common_reqs = ::std::conjunction<
    // Less-than comparable to find the maximum degree
    // of the terms.
    is_less_than_comparable<::std::add_lvalue_reference_t<const DegreeType>>,
    // Ctible from int to init the degree
    // of an empty series.
    ::std::is_constructible<DegreeType, int>,
    // Returnable.
    is_returnable<DegreeType>,
    // NOTE: require a semi-regular type,
    // it's just easier to reason about.
    is_semi_regular<DegreeType>>;

template <typename DegreeType>
inline constexpr bool series_default_degree_type_common_reqs_v
    = series_default_degree_type_common_reqs<DegreeType>::value;

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

                // NOTE: the common reqs take care of ensuring that degree_t is detected
                // (because nonesuch is not lt-comparable etc.).
                if constexpr (series_default_degree_type_common_reqs_v<degree_t>) {
                    return ::std::make_pair(1, detail::type_c<degree_t>{});
                } else {
                    return failure;
                }
            } else if constexpr (cf_has_degree) {
                // Only the coefficient is with degree.
                using degree_t = DegreeT<const cf_t &>;

                if constexpr (series_default_degree_type_common_reqs_v<degree_t>) {
                    // degree_t supports operator<, it can be constructed from int,
                    // it is returnable and semi-regular (hence, move-assignable).
                    return ::std::make_pair(2, detail::type_c<degree_t>{});
                } else {
                    return failure;
                }
            } else {
                // Only the key is with degree.
                using degree_t = KeyDegreeT<const key_t &>;

                if constexpr (series_default_degree_type_common_reqs_v<degree_t>) {
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
#if !defined(_MSC_VER) || defined(__clang__)
                // Both coefficient and key with degree.
                using key_deg_t = decltype(::obake::key_degree(p.first, *ss));
                using cf_deg_t = decltype(::obake::degree(p.second));

                if constexpr (::std::conjunction_v<is_integral<key_deg_t>, is_integral<cf_deg_t>>) {
                    // Both key and coefficient return an integral degree.
                    // Determine the common type (via addition) and then
                    // do the summation using checked integral arithmetics.
                    using deg_add_t = decltype(::obake::key_degree(p.first, *ss) + ::obake::degree(p.second));

                    return detail::safe_int_add<deg_add_t>(::obake::key_degree(p.first, *ss),
                                                           ::obake::degree(p.second));
                } else {
#endif
                    return ::obake::key_degree(p.first, *ss) + ::obake::degree(p.second);
#if !defined(_MSC_VER) || defined(__clang__)
                }
#endif
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
requires(series_default_degree_impl::algo<T> != 0) inline constexpr auto degree<T>
#else
inline constexpr auto degree<T, ::std::enable_if_t<series_default_degree_impl::algo<T> != 0>>
#endif
    = series_default_degree_impl{};

// Helper to construct a vector of total degrees
// from a range of terms using the series_default_degree_impl
// machinery. T is the series type which the terms
// in the range refer to. The 'parallel' flag establishes
// if the construction of the vector of degrees should
// be done in a parallel fashion. 'It' must be a random-access iterator.
template <typename T, typename It>
inline auto make_degree_vector(It begin, It end, const symbol_set &ss, bool parallel)
{
    static_assert(is_random_access_iterator_v<It>);

    using d_impl = series_default_degree_impl;
    using deg_t = decltype(d_impl::d_extractor<T>{&ss}(*begin));

    // Build the degree extractor.
    const auto d_ex = d_impl::d_extractor<T>{&ss};

    if (parallel) {
        ::std::vector<deg_t> retval;
        // NOTE: we require deg_t to be a semi-regular type,
        // thus it is def-constructible.
        retval.resize(::obake::safe_cast<decltype(retval.size())>(end - begin));

        ::tbb::parallel_for(::tbb::blocked_range(begin, end), [&retval, &d_ex, begin](const auto &range) {
            for (auto it = range.begin(); it != range.end(); ++it) {
                retval[static_cast<decltype(retval.size())>(it - begin)] = d_ex(*it);
            }
        });

        return retval;
    } else {
        return ::std::vector<deg_t>(::boost::make_transform_iterator(begin, d_ex),
                                    ::boost::make_transform_iterator(end, d_ex));
    }
}

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
#if !defined(_MSC_VER) || defined(__clang__)
                // Both coefficient and key with partial degree.
                using key_deg_t = decltype(::obake::key_p_degree(p.first, *si, *ss));
                using cf_deg_t = decltype(::obake::p_degree(p.second, *s));

                if constexpr (::std::conjunction_v<is_integral<key_deg_t>, is_integral<cf_deg_t>>) {
                    // Both key and coefficient return an integral degree.
                    // Determine the common type (via addition) and then
                    // do the summation using checked integral arithmetics.
                    using deg_add_t
                        = decltype(::obake::key_p_degree(p.first, *si, *ss) + ::obake::p_degree(p.second, *s));

                    return detail::safe_int_add<deg_add_t>(::obake::key_p_degree(p.first, *si, *ss),
                                                           ::obake::p_degree(p.second, *s));
                } else {
#endif
                    return ::obake::key_p_degree(p.first, *si, *ss) + ::obake::p_degree(p.second, *s);
#if !defined(_MSC_VER) || defined(__clang__)
                }
#endif
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
requires(series_default_p_degree_impl::algo<T> != 0) inline constexpr auto p_degree<T>
#else
inline constexpr auto p_degree<T, ::std::enable_if_t<series_default_p_degree_impl::algo<T> != 0>>
#endif
    = series_default_p_degree_impl{};

// Helper to construct a vector of partial degrees
// from a range of terms using the series_default_p_degree_impl
// machinery. T is the series type which the terms
// in the range refer to. The 'parallel' flag establishes
// if the construction of the vector of degrees should
// be done in a parallel fashion. 'It' must be a random-access iterator.
template <typename T, typename It>
inline auto make_p_degree_vector(It begin, It end, const symbol_set &ss, const symbol_set &s, bool parallel)
{
    static_assert(is_random_access_iterator_v<It>);

    using d_impl = series_default_p_degree_impl;

    // Turn the list of symbols into a set of indices.
    const auto si = detail::ss_intersect_idx(s, ss);

    using deg_t = decltype(d_impl::d_extractor<T>{&s, &si, &ss}(*begin));

    // Build the degree extractor.
    const auto d_ex = d_impl::d_extractor<T>{&s, &si, &ss};

    if (parallel) {
        ::std::vector<deg_t> retval;
        // NOTE: we require deg_t to be a semi-regular type,
        // thus it is def-constructible.
        retval.resize(::obake::safe_cast<decltype(retval.size())>(end - begin));

        ::tbb::parallel_for(::tbb::blocked_range(begin, end), [&retval, &d_ex, begin](const auto &range) {
            for (auto it = range.begin(); it != range.end(); ++it) {
                retval[static_cast<decltype(retval.size())>(it - begin)] = d_ex(*it);
            }
        });

        return retval;
    } else {
        return ::std::vector<deg_t>(::boost::make_transform_iterator(begin, d_ex),
                                    ::boost::make_transform_iterator(end, d_ex));
    }
}

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
                              is_in_place_addable<::std::add_lvalue_reference_t<ret_t>, ret_t>,
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
requires(series_default_evaluate_impl::algo<T, U> != 0) inline constexpr auto evaluate<T, U>
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
inline series<K, C, Tag> filtered_impl(const series<K, C, Tag> &s, const F &f)
{
    // Init the return value. Same symbol set
    // and same number of segments as s.
    series<K, C, Tag> retval;
    retval.set_symbol_set_fw(s.get_symbol_set_fw());
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

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct filtered_msvc {
    template <typename T, typename F>
    constexpr auto operator()(T &&s, const F &f) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::filtered_impl(::std::forward<T>(s), f))
};

inline constexpr auto filtered = filtered_msvc{};

#else

// NOTE: do we need a concept/type trait for this? See also the testing.
// NOTE: force const reference passing for f as a hint
// that the implementation may be parallel.
inline constexpr auto filtered =
    [](auto &&s, const auto &f) OBAKE_SS_FORWARD_LAMBDA(detail::filtered_impl(::std::forward<decltype(s)>(s), f));

#endif

namespace detail
{

template <typename K, typename C, typename Tag, typename F,
          ::std::enable_if_t<::std::is_convertible_v<detected_t<term_filter_return_t, F, K, C, Tag>, bool>, int> = 0>
inline void filter_impl(series<K, C, Tag> &s, const F &f)
{
    // Do the filtering table by table.
    // NOTE: this can easily be parallelised.
    for (auto &table : s._get_s_table()) {
        const auto it_f = table.end();

        for (auto it = table.begin(); it != it_f;) {
            // NOTE: abseil's flat_hash_map returns void on erase(),
            // thus we need to increase 'it' before possibly erasing.
            // erase() does not cause rehash and thus will not invalidate
            // any other iterator apart from the one being erased.
            if (f(::std::as_const(*it))) {
                ++it;
            } else {
                table.erase(it++);
            }
        }
    }
}

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

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
// NOTE: perhaps we could eventually change the implementation
// to return a reference to s.
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

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

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
