// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POWER_SERIES_POWER_SERIES_HPP
#define OBAKE_POWER_SERIES_POWER_SERIES_HPP

#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <boost/flyweight/flyweight.hpp>
#include <boost/flyweight/hashed_factory.hpp>

#include <obake/config.hpp>
#include <obake/detail/fw_utils.hpp>
#include <obake/hash.hpp>
#include <obake/math/degree.hpp>
#include <obake/math/p_degree.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace power_series
{

namespace detail
{

// Struct to represent the absence
// of truncation in a power series.
struct no_truncation {
};

// Need equality operator for use in the fw hashed container.
inline bool operator==(const no_truncation &, const no_truncation &)
{
    return true;
}

// The truncation state.
template <typename T>
using trunc_t = ::std::variant<no_truncation, T, ::std::pair<T, symbol_set>>;

// Hasher for trunc_t.
template <typename T>
struct trunc_t_hasher {
    ::std::size_t operator()(const trunc_t<T> &t) const
    {
        return ::std::visit(
            [](const auto &v) -> ::std::size_t {
                using type = remove_cvref_t<decltype(v)>;

                if constexpr (::std::is_same_v<type, no_truncation>) {
                    return 0;
                } else if constexpr (::std::is_same_v<type, T>) {
                    return ::obake::hash(v);
                } else {
                    // NOTE: mix the hashes of degree and symbol set.
                    return ::obake::hash(v.first) + ::obake::detail::ss_fw_hasher{}(v.second);
                }
            },
            t);
    }
};

// Comparer for trunc_t.
// NOTE: the purpose of this helper is to enforce
// the comparison of trunc_t objects via const refs.
template <typename T>
struct trunc_t_comparer {
    bool operator()(const trunc_t<T> &t1, const trunc_t<T> &t2) const
    {
        return t1 == t2;
    }
};

// Definition of the trunc_t flyweight.
template <typename T>
using trunc_t_fw
    = ::boost::flyweight<trunc_t<T>, ::boost::flyweights::hashed_factory<trunc_t_hasher<T>, trunc_t_comparer<T>>,
                         ::obake::detail::fw_holder>;

// NOTE: this helper returns a copy of a global thread-local
// default-cted trunc_t_fw object. This is useful for fast
// default construction of classes having trunc_t_fw
// as a data member.
template <typename T>
inline trunc_t_fw<T> trunc_t_fw_default()
{
    static thread_local trunc_t_fw<T> ret{};

    return ret;
}

} // namespace detail

template <typename T>
struct tag {
    detail::trunc_t_fw<T> trunc = detail::trunc_t_fw_default<T>();
};

// Stream operator for the tag.
template <typename T>
inline ::std::ostream &operator<<(::std::ostream &os, const tag<T> &t)
{
    return os << "Series type: power series\n"
              << ::std::visit(
                     [](const auto &v) -> ::std::string {
                         using type = remove_cvref_t<decltype(v)>;

                         if constexpr (::std::is_same_v<type, detail::no_truncation>) {
                             return "Truncation: none";
                         } else {
                             ::std::ostringstream oss;

                             if constexpr (::std::is_same_v<type, T>) {
                                 oss << v;
                                 return "Truncation degree: " + oss.str();
                             } else {
                                 oss << v.first;
                                 return "Partial truncation degree: " + oss.str() + ", "
                                        + ::obake::detail::to_string(v.second);
                             }
                         }
                     },
                     t.trunc.get());
}

} // namespace power_series

// Coefficient type for a power_series:
// - must satisfy is_cf,
// - must not be with degree
//   (via const lvalue).
// NOTE: the lack of a degree ensures
// that the default series degree
// machinery considers only the degree
// of the key for the series degree
// computation, truncation, etc.
template <typename C>
using is_power_series_cf
    = ::std::conjunction<is_cf<C>, ::std::negation<is_with_degree<::std::add_lvalue_reference_t<const C>>>>;

template <typename C>
inline constexpr bool is_power_series_cf_v = is_power_series_cf<C>::value;

namespace detail
{

// A small helper to fetch the (partial) degree type
// of the key of a power series (if it exists).
// Used to reduce typing below.
template <typename K>
using psk_deg_t = detected_t<detail::key_degree_t, ::std::add_lvalue_reference_t<const K>>;

template <typename K>
using psk_pdeg_t = detected_t<detail::key_p_degree_t, ::std::add_lvalue_reference_t<const K>>;

} // namespace detail

// Key type for a power_series:
// - must satisfy is_key,
// - must have a (partial) key degree
//   type (via const lvalue) satisfying
//   the requirements of the default
//   (partial) degree machinery
//   for series,
// - the partial and total degree types
//   must be identical,
// - the degree type must be hashable,
//   equality-comparable and
//   stream-insertable (via const lvalue refs).
// NOTE: the default series degree machinery
// already checks that the degree is a
// semi-regular type (which we want for use
// in a variant, and for general copy/move
// operations), and it also ensures
// truncability via the polynomial implementation
// (which we need to implement the truncate()
// helper in power_series).
// TODO do we have a truncate() helper?
template <typename K>
using is_power_series_key
    = ::std::conjunction<is_key<K>,
                         // NOTE: nonesuch is not semi-regular, hence
                         // the common reqs also take care of detection
                         // for the degree types.
                         customisation::internal::series_default_degree_type_common_reqs<detail::psk_deg_t<K>>,
                         customisation::internal::series_default_degree_type_common_reqs<detail::psk_pdeg_t<K>>,
                         ::std::is_same<detail::psk_deg_t<K>, detail::psk_pdeg_t<K>>, is_hashable<detail::psk_deg_t<K>>,
                         is_equality_comparable<detail::psk_deg_t<K>>, is_stream_insertable<detail::psk_deg_t<K>>>;

template <typename K>
inline constexpr bool is_power_series_key_v = is_power_series_key<K>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename C>
OBAKE_CONCEPT_DECL power_series_cf = is_power_series_cf_v<C>;

template <typename K>
OBAKE_CONCEPT_DECL power_series_key = is_power_series_key_v<K>;

#endif

#if defined(OBAKE_HAVE_CONCEPTS)
template <power_series_key K, power_series_cf C>
#else
template <typename K, typename C,
          typename = ::std::enable_if_t<::std::conjunction_v<is_power_series_key<K>, is_power_series_cf<C>>>>
#endif
using p_series = series<K, C, power_series::tag<detail::psk_deg_t<K>>>;

template <
    typename K, typename C, typename T,
    ::std::enable_if_t<is_safely_castable_v<::std::add_lvalue_reference_t<const T>, detail::psk_deg_t<K>>, int> = 0>
inline p_series<K, C> &set_truncation(p_series<K, C> &ps, const T &d)
{
    ps.tag().trunc = power_series::detail::trunc_t<detail::psk_deg_t<K>>(::obake::safe_cast<detail::psk_deg_t<K>>(d));

    return ps;
}

template <
    typename K, typename C, typename T,
    ::std::enable_if_t<is_safely_castable_v<::std::add_lvalue_reference_t<const T>, detail::psk_deg_t<K>>, int> = 0>
inline p_series<K, C> &set_truncation(p_series<K, C> &ps, const T &d, symbol_set ss)
{
    ps.tag().trunc = power_series::detail::trunc_t<detail::psk_deg_t<K>>(
        ::std::pair{::obake::safe_cast<detail::psk_deg_t<K>>(d), ::std::move(ss)});

    return ps;
}

template <typename K, typename C>
inline p_series<K, C> &unset_truncation(p_series<K, C> &ps)
{
    ps.tag().trunc = power_series::detail::trunc_t_fw_default<detail::psk_deg_t<K>>();

    return ps;
}

} // namespace obake

#endif
