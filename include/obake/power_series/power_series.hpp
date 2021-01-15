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
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <boost/flyweight/flyweight.hpp>
#include <boost/flyweight/hashed_factory.hpp>

#include <fmt/format.h>

#include <obake/config.hpp>
#include <obake/detail/fw_utils.hpp>
#include <obake/detail/it_diff_check.hpp>
#include <obake/detail/make_array.hpp>
#include <obake/detail/ss_func_forward.hpp>
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
// (which we need to implement the truncate() function).
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

namespace detail
{

template <typename T>
struct is_any_p_series_impl : ::std::false_type {
};

template <typename K, typename C>
struct is_any_p_series_impl<p_series<K, C>> : ::std::true_type {
};

} // namespace detail

// Detect power series.
template <typename T>
using is_any_p_series = detail::is_any_p_series_impl<T>;

template <typename T>
inline constexpr bool is_any_p_series_v = is_any_p_series<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL any_p_series = is_any_p_series_v<T>;

#endif

namespace power_series
{

// Implementation of (partial) degree truncation for power series.
template <
    typename K, typename C, typename T,
    ::std::enable_if_t<is_less_than_comparable_v<::std::add_lvalue_reference_t<const T>, ::obake::detail::psk_deg_t<K>>,
                       int> = 0>
inline void truncate_degree(p_series<K, C> &ps, const T &d)
{
    // Use the default functor for the extraction of the term degree.
    // NOTE: d_impl is assured to work thanks to the concept
    // requirements for ps key. The only extra bit we need in this function
    // is to be able to compare d to the degree type, which
    // is checked above.
    using d_impl = customisation::internal::series_default_degree_impl;

    // Implement on top of filter().
    ::obake::filter(ps, [deg_ext = d_impl::d_extractor<p_series<K, C>>{&ps.get_symbol_set()}, &d](const auto &t) {
        return !(d < deg_ext(t));
    });
}

template <
    typename K, typename C, typename T,
    ::std::enable_if_t<is_less_than_comparable_v<::std::add_lvalue_reference_t<const T>, ::obake::detail::psk_deg_t<K>>,
                       int> = 0>
inline void truncate_p_degree(p_series<K, C> &ps, const T &d, const symbol_set &s)
{
    // Use the default functor for the extraction of the term degree.
    // NOTE: d_impl is assured to work thanks to the concept
    // requirements for ps key. The only extra bit we need in this function
    // is to be able to compare d to the degree type, which
    // is checked above.
    using d_impl = customisation::internal::series_default_p_degree_impl;

    // Extract the symbol indices.
    const auto &ss = ps.get_symbol_set();
    const auto si = ::obake::detail::ss_intersect_idx(s, ss);

    // Implement on top of filter().
    ::obake::filter(ps, [deg_ext = d_impl::d_extractor<p_series<K, C>>{&s, &si, &ss}, &d](const auto &t) {
        return !(d < deg_ext(t));
    });
}

} // namespace power_series

// Set total degree truncation.
template <
    typename K, typename C, typename T,
    ::std::enable_if_t<is_safely_castable_v<::std::add_lvalue_reference_t<const T>, detail::psk_deg_t<K>>, int> = 0>
inline p_series<K, C> &set_truncation(p_series<K, C> &ps, const T &d)
{
    // Convert safely d to the degree type.
    const auto deg = ::obake::safe_cast<detail::psk_deg_t<K>>(d);

    try {
        // Proceed with the truncation.
        // NOTE: truncate_degree() is ensured to work because deg
        // is the degree type, which is required to be less-than
        // comparable in the ps requirements.
        power_series::truncate_degree(ps, deg);

        // Set the truncation level in ps.
        ps.tag().trunc = power_series::detail::trunc_t<detail::psk_deg_t<K>>(deg);

        // LCOV_EXCL_START
    } catch (...) {
        // NOTE: if anything goes wrong, make sure we clear
        // up ps before rethrowing. This will clear up terms
        // and symbol set, and will reset the truncation
        // tag to its def-cted state (i.e., no truncation).
        ps.clear();

        throw;
    }
    // LCOV_EXCL_STOP

    return ps;
}

template <
    typename K, typename C, typename T,
    ::std::enable_if_t<is_safely_castable_v<::std::add_lvalue_reference_t<const T>, detail::psk_deg_t<K>>, int> = 0>
inline p_series<K, C> &set_truncation(p_series<K, C> &ps, const T &d, symbol_set ss)
{
    // Convert safely d to the degree type.
    const auto deg = ::obake::safe_cast<detail::psk_deg_t<K>>(d);

    try {
        // Proceed with the truncation.
        // NOTE: truncate_p_degree() is ensured to work because deg
        // is the degree type, which is required to be less-than
        // comparable in the ps requirements.
        power_series::truncate_p_degree(ps, deg, ss);

        // Set the truncation level in ps.
        ps.tag().trunc = power_series::detail::trunc_t<detail::psk_deg_t<K>>(::std::pair{deg, ::std::move(ss)});

        // LCOV_EXCL_START
    } catch (...) {
        // NOTE: if anything goes wrong, make sure we clear
        // up ps before rethrowing. This will clear up terms
        // and symbol set, and will reset the truncation
        // tag to its def-cted state (i.e., no truncation).
        ps.clear();

        throw;
    }
    // LCOV_EXCL_STOP

    return ps;
}

template <typename K, typename C>
inline p_series<K, C> &unset_truncation(p_series<K, C> &ps)
{
    ps.tag().trunc = power_series::detail::trunc_t_fw_default<detail::psk_deg_t<K>>();

    return ps;
}

template <typename K, typename C>
inline const auto &get_truncation(const p_series<K, C> &ps)
{
    return ps.tag().trunc.get();
}

// Factory functions for power series.
namespace detail
{

// Enabler for make_p_series():
// - T must be a power series,
// - std::string can be constructed from each input Args,
// - ps key can be constructed from a const int * range,
// - ps cf can be constructed from an integral literal.
template <typename T, typename... Args>
using make_p_series_supported
    = ::std::conjunction<::std::integral_constant<bool, (sizeof...(Args) > 0u)>, is_any_p_series<T>,
                         ::std::is_constructible<::std::string, const Args &>...,
                         ::std::is_constructible<series_key_t<T>, const int *, const int *>,
                         ::std::is_constructible<series_cf_t<T>, int>>;

template <typename T, typename... Args>
using make_p_series_enabler = ::std::enable_if_t<make_p_series_supported<T, Args...>::value, int>;

// Overload without a symbol set, no truncation.
template <typename T, typename... Args, make_p_series_enabler<T, Args...> = 0>
inline auto make_p_series_impl(const Args &...names)
{
    auto make_p_series = [](const auto &n) {
        using str_t = remove_cvref_t<decltype(n)>;

        // Init the retval, assign a symbol set containing only n.
        T retval;
        if constexpr (::std::is_same_v<str_t, ::std::string>) {
            retval.set_symbol_set(symbol_set{n});
        } else {
            retval.set_symbol_set(symbol_set{::std::string(n)});
        }

        constexpr int arr[] = {1};

        // Create and add a new term.
        retval.add_term(series_key_t<T>(&arr[0], &arr[0] + 1), 1);

        return retval;
    };

    return detail::make_array(make_p_series(names)...);
}

// Overload with a symbol set, no truncation.
template <typename T, typename... Args, make_p_series_enabler<T, Args...> = 0>
inline auto make_p_series_impl(const symbol_set &ss, const Args &...names)
{
    // Create a temp vector of ints which we will use to
    // init the keys.
    ::std::vector<int> tmp(::obake::safe_cast<::std::vector<int>::size_type>(ss.size()));

    // Create the fw version of the symbol set.
    const detail::ss_fw ss_fw(ss);

    auto make_p_series = [&ss_fw, &ss, &tmp](const auto &n) {
        using str_t = remove_cvref_t<decltype(n)>;

        // Fetch a const reference to either the original
        // std::string object n, or to a string temporary
        // created from it.
        const auto &s = [&n]() -> decltype(auto) {
            if constexpr (::std::is_same_v<str_t, ::std::string>) {
                return n;
            } else {
                return ::std::string(n);
            }
        }();

        // Init the retval, assign the symbol set.
        T retval;
        retval.set_symbol_set_fw(ss_fw);

        // Try to locate s within the symbol set.
        const auto it = ss.find(s);
        if (obake_unlikely(it == ss.end() || *it != s)) {
            using namespace ::fmt::literals;

            obake_throw(::std::invalid_argument,
                        "Cannot create a power series with symbol set {} from the "
                        "generator '{}': the generator is not in the symbol set"_format(detail::to_string(ss), s));
        }

        // Set to 1 the exponent of the corresponding generator.
        tmp[static_cast<::std::vector<int>::size_type>(ss.index_of(it))] = 1;

        // Create and add a new term.
        // NOTE: at least for some monomial types (e.g., packed monomial),
        // we will be computing the iterator difference when constructing from
        // a range. Make sure we can safely represent the size of tmp via
        // iterator difference.
        ::obake::detail::it_diff_check<decltype(::std::as_const(tmp).data())>(tmp.size());
        retval.add_term(series_key_t<T>(::std::as_const(tmp).data(), ::std::as_const(tmp).data() + tmp.size()), 1);

        // Set back to zero the exponent that was previously set to 1.
        tmp[static_cast<::std::vector<int>::size_type>(ss.index_of(it))] = 0;

        return retval;
    };

    return detail::make_array(make_p_series(names)...);
}

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

template <typename T>
struct make_p_series_msvc {
    template <typename... Args>
    constexpr auto operator()(const Args &...args) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::make_p_series_impl<T>(args...))
};

template <typename T>
inline constexpr auto make_p_series = make_p_series_msvc<T>{};

#else

// Power series creation functor.
template <typename T>
inline constexpr auto make_p_series
    = [](const auto &...args) OBAKE_SS_FORWARD_LAMBDA(detail::make_p_series_impl<T>(args...));

#endif

} // namespace obake

#endif
