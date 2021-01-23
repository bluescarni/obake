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
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/utility.hpp>

#include <fmt/format.h>

#include <obake/detail/fw_utils.hpp>
#include <obake/detail/it_diff_check.hpp>
#include <obake/detail/make_array.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/hash.hpp>
#include <obake/math/degree.hpp>
#include <obake/math/p_degree.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/s11n.hpp>
#include <obake/series.hpp>
#include <obake/symbols.hpp>
#include <obake/tex_stream_insert.hpp>
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
    template <typename Archive>
    void serialize(Archive &, unsigned)
    {
    }
};

// Need equality operator for use in the fw hashed container.
inline bool operator==(const no_truncation &, const no_truncation &)
{
    return true;
}

inline bool operator!=(const no_truncation &, const no_truncation &)
{
    return false;
}

// The truncation state.
template <typename T>
using trunc_t = ::std::variant<no_truncation, T, ::std::pair<T, symbol_set>>;

} // namespace detail

} // namespace power_series

} // namespace obake

// Disable tracking for no_truncation.
BOOST_CLASS_TRACKING(::obake::power_series::detail::no_truncation, ::boost::serialization::track_never)

// Serialisation for trunc_t.
// NOTE: trunc_t is a std::variant, perhaps in the future
// Boost.serialization will provide an implementation. Note that
// even if Boost eventually does provide an implementation, this
// code will still be valid as we are specialising for a variant
// containing our no_truncation class (thus these specialised
// functions should be picked over the general-purpose implementation).
namespace boost::serialization
{

template <class Archive, typename T>
inline void save(Archive &ar, const ::obake::power_series::detail::trunc_t<T> &t, unsigned)
{
    // Save the index.
    ar << t.index();

    // Save the value.
    ::std::visit([&ar](const auto &v) { ar << v; }, t);
}

template <class Archive, typename T>
inline void load(Archive &ar, ::obake::power_series::detail::trunc_t<T> &t, unsigned)
{
    // Fetch the index.
    ::std::size_t idx;
    ar >> idx;

    // Fetch the value.
    switch (idx) {
        case 0u: {
            ::obake::power_series::detail::no_truncation nt;
            ar >> nt;
            t = ::std::move(nt);

            break;
        }
        case 1u: {
            // NOTE: for the (partial) degree
            // truncation case, we mimick the
            // implementation of s11n for
            // Boost.variant, which informs
            // the archive that the deserialized
            // object has been moved into the variant.
            // We don't need this for no_truncation,
            // as this extra step is needed only for
            // object tracking, which is disabled for
            // no_truncation.
            T n;
            ar >> n;
            t = ::std::move(n);

            ar.reset_object_address(&::std::get<1>(t), &n);

            break;
        }
        case 2u: {
            ::std::pair<T, ::obake::symbol_set> p;
            ar >> p;
            t = ::std::move(p);

            ar.reset_object_address(&::std::get<2>(t), &p);

            break;
        }
        // LCOV_EXCL_START
        default: {
            using namespace ::fmt::literals;

            obake_throw(::std::invalid_argument, "The deserialisation of a truncation limit for a power"
                                                 "series produced the invalid variant index {}"_format(idx));
        }
            // LCOV_EXCL_STOP
    }
}

// NOTE: cannot use directly the split_free macro
// due to the presence of the template.
template <class Archive, typename T>
inline void serialize(Archive &ar, ::obake::power_series::detail::trunc_t<T> &t, unsigned file_version)
{
    split_free(ar, t, file_version);
}

// Disable tracking for trunc_t.
template <typename T>
struct tracking_level<::obake::power_series::detail::trunc_t<T>>
    : ::obake::detail::s11n_no_tracking<::obake::power_series::detail::trunc_t<T>> {
};

} // namespace boost::serialization

namespace obake
{

namespace power_series
{

namespace detail
{

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

} // namespace detail

// The power series tag.
template <typename T>
struct tag {
    detail::trunc_t_fw<T> trunc;

    template <typename Archive>
    void save(Archive &ar, unsigned) const
    {
        ar << trunc.get();
    }
    template <typename Archive>
    void load(Archive &ar, unsigned)
    {
        // NOTE: instead of using flyweight's
        // s11n support, we go through a temporary
        // trunc_t. The reason here is that
        // fw's s11n support triggers a crash at
        // program shutdown, seemingly related
        // to some static init order issue. For now,
        // let's just do it like this, even if it is
        // suboptimal wrt archive size. Perhaps when
        // this starts to matter we can revisit the
        // issue.
        // NOTE: no need to reset the object address,
        // object tracking is disabled for trunc_t.
        detail::trunc_t<T> tmp;
        ar >> tmp;

        trunc = tmp;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

// Implement equality for the tag, so that series'
// equality operator can use it.
template <typename T>
inline bool operator==(const tag<T> &t0, const tag<T> &t1)
{
    return t0.trunc == t1.trunc;
}

template <typename T>
inline bool operator!=(const tag<T> &t0, const tag<T> &t1)
{
    return t0.trunc != t1.trunc;
}

// Implement the swap primitive for tag.
template <typename T>
inline void swap(tag<T> &t0, tag<T> &t1) noexcept
{
    using ::std::swap;

    swap(t0.trunc, t1.trunc);
}

// Implement the hash primitive for the tag.
// NOTE: this is used in series' pow() caching machinery.
template <typename T>
inline ::std::size_t hash(const tag<T> &t)
{
    return detail::trunc_t_hasher<T>{}(t.trunc.get());
}

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
                             oss.exceptions(::std::ios_base::failbit | ::std::ios_base::badbit);

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
concept power_series_cf = Cf<C> && !WithDegree<const C &>;

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
// operations).
template <typename K>
concept power_series_key
    = Key<K> &&customisation::internal::series_default_degree_type_common_reqs<detail::psk_deg_t<K>>::value
        &&customisation::internal::series_default_degree_type_common_reqs<detail::psk_pdeg_t<K>>::value
            && ::std::is_same_v<detail::psk_deg_t<K>, detail::psk_pdeg_t<K>> &&Hashable<const detail::psk_deg_t<K> &>
                &&EqualityComparable<const detail::psk_deg_t<K> &> &&StreamInsertable<const detail::psk_deg_t<K> &>;

// Definition of the power series class.
template <power_series_key K, power_series_cf C>
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

// Detect any power series.
template <typename T>
concept any_p_series = detail::is_any_p_series_impl<T>::value;

namespace power_series
{

// Implementation of (partial) degree truncation for power series.
template <typename K, typename C, typename T>
requires LessThanComparable<const T &, ::obake::detail::psk_deg_t<K>> inline void truncate_degree(p_series<K, C> &ps,
                                                                                                  const T &d)
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

template <typename K, typename C, typename T>
requires LessThanComparable<const T &, ::obake::detail::psk_deg_t<K>> inline void
truncate_p_degree(p_series<K, C> &ps, const T &d, const symbol_set &s)
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

namespace detail
{

// Set total degree truncation.
template <typename K, typename C, typename T>
requires SafelyCastable<const T &, detail::psk_deg_t<K>> inline p_series<K, C> &set_truncation_impl(p_series<K, C> &ps,
                                                                                                    const T &d)
{
    // Convert safely d to the degree type.
    const auto deg = ::obake::safe_cast<detail::psk_deg_t<K>>(d);

    try {
        // Proceed with the truncation.
        // NOTE: truncate_degree() is ensured to work because deg
        // is the degree type, which is required to be less-than
        // comparable in the ps requirements.
        power_series::truncate_degree(ps, deg);

        // Set the truncation policy/level in ps.
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

// Set partial degree truncation.
template <typename K, typename C, typename T>
requires SafelyCastable<const T &, detail::psk_deg_t<K>> inline p_series<K, C> &
set_truncation_impl(p_series<K, C> &ps, const T &d, symbol_set ss)
{
    // Convert safely d to the degree type.
    const auto deg = ::obake::safe_cast<detail::psk_deg_t<K>>(d);

    try {
        // Proceed with the truncation.
        // NOTE: truncate_p_degree() is ensured to work because deg
        // is the degree type, which is required to be less-than
        // comparable in the ps requirements.
        power_series::truncate_p_degree(ps, deg, ss);

        // Set the truncation policy/level in ps.
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

} // namespace detail

// Set truncation.
inline constexpr auto set_truncation = []<typename K, typename C, typename... Args>(p_series<K, C> & ps, Args &&...args)
    OBAKE_SS_FORWARD_LAMBDA(detail::set_truncation_impl(ps, ::std::forward<Args>(args)...));

// Unset truncation.
inline constexpr auto unset_truncation = []<typename K, typename C>(p_series<K, C> &ps) -> p_series<K, C> & {
    ps.tag().trunc = power_series::detail::trunc_t<detail::psk_deg_t<K>>();

    return ps;
};

// Get the truncation.
inline constexpr auto get_truncation = []<typename K, typename C>(const p_series<K, C> &ps) -> const auto &
{
    return ps.tag().trunc.get();
};

// Truncate according to the current truncation policy and level.
inline constexpr auto truncate = []<typename K, typename C>(p_series<K, C> &ps) {
    ::std::visit(
        [&ps](const auto &v) {
            using type = remove_cvref_t<decltype(v)>;

            if constexpr (::std::is_same_v<type, power_series::detail::no_truncation>) {
            } else if constexpr (::std::is_same_v<type, detail::psk_deg_t<K>>) {
                power_series::truncate_degree(ps, v);
            } else {
                power_series::truncate_p_degree(ps, v.first, v.second);
            }
        },
        ::obake::get_truncation(ps));
};

// Factory functions for power series.
namespace detail
{

// Enabler for make_p_series():
// - need at least 1 Arg,
// - T must be a power series,
// - std::string can be constructed from each input Args,
// - ps key can be constructed from a const int * range,
// - ps cf can be constructed from an integral literal.
template <typename T, typename... Args>
concept make_p_series_supported
    = (sizeof...(Args) > 0u)
      && (any_p_series<T>)&&(... && ::std::is_constructible_v<::std::string, const Args &>)&&::std::is_constructible_v<
          series_key_t<T>, const int *, const int *> && ::std::is_constructible_v<series_cf_t<T>, int>;

// Overload without a symbol set, no truncation.
template <typename T, typename... Args>
requires make_p_series_supported<T, Args...> inline auto make_p_series_impl(const Args &...names)
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
template <typename T, typename... Args>
requires make_p_series_supported<T, Args...> inline auto make_p_series_impl(const symbol_set &ss, const Args &...names)
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

// Power series creation functor, no truncation.
template <typename T>
inline constexpr auto make_p_series
    = [](const auto &...args) OBAKE_SS_FORWARD_LAMBDA(detail::make_p_series_impl<T>(args...));

namespace detail
{

template <typename T, typename U, typename... Args>
concept make_p_series_t_supported
    = make_p_series_supported<T, Args...> &&SafelyCastable<const U &, psk_deg_t<series_key_t<T>>>;

// Overload without a symbol set, total truncation.
template <typename T, typename U, typename... Args>
requires make_p_series_t_supported<T, U, Args...> inline auto make_p_series_t_impl(const U &d, const Args &...names)
{
    // Convert d to the degree type.
    const auto deg = ::obake::safe_cast<psk_deg_t<series_key_t<T>>>(d);

    auto make_p_series = [&deg](const auto &n) {
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

        // Set the truncation.
        ::obake::set_truncation(retval, deg);

        return retval;
    };

    return detail::make_array(make_p_series(names)...);
}

// Overload with a symbol set, total truncation.
template <typename T, typename U, typename... Args>
requires make_p_series_t_supported<T, U, Args...> inline auto make_p_series_t_impl(const symbol_set &ss, const U &d,
                                                                                   const Args &...names)
{
    // Convert d to the degree type.
    const auto deg = ::obake::safe_cast<psk_deg_t<series_key_t<T>>>(d);

    // Create a temp vector of ints which we will use to
    // init the keys.
    ::std::vector<int> tmp(::obake::safe_cast<::std::vector<int>::size_type>(ss.size()));

    // Create the fw version of the symbol set.
    const detail::ss_fw ss_fw(ss);

    auto make_p_series = [&deg, &ss_fw, &ss, &tmp](const auto &n) {
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

        // Set the truncation.
        ::obake::set_truncation(retval, deg);

        // Set back to zero the exponent that was previously set to 1.
        tmp[static_cast<::std::vector<int>::size_type>(ss.index_of(it))] = 0;

        return retval;
    };

    return detail::make_array(make_p_series(names)...);
}

} // namespace detail

// Power series creation functor, total degree truncation.
template <typename T>
inline constexpr auto make_p_series_t
    = [](const auto &...args) OBAKE_SS_FORWARD_LAMBDA(detail::make_p_series_t_impl<T>(args...));

namespace detail
{

// Overload without a symbol set, partial truncation.
template <typename T, typename U, typename... Args>
// NOTE: for partial degree truncation we can re-use the concepts
// used in total degree truncation.
requires make_p_series_t_supported<T, U, Args...> inline auto make_p_series_p_impl(const U &d, const symbol_set &tss,
                                                                                   const Args &...names)
{
    // Convert d to the degree type.
    const auto deg = ::obake::safe_cast<psk_deg_t<series_key_t<T>>>(d);

    auto make_p_series = [&deg, &tss](const auto &n) {
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

        // Set the truncation.
        ::obake::set_truncation(retval, deg, tss);

        return retval;
    };

    return detail::make_array(make_p_series(names)...);
}

// Overload with a symbol set, partial truncation.
template <typename T, typename U, typename... Args>
// NOTE: for partial degree truncation we can re-use the concepts
// used in total degree truncation.
requires make_p_series_t_supported<T, U, Args...> inline auto
make_p_series_p_impl(const symbol_set &ss, const U &d, const symbol_set &tss, const Args &...names)
{
    // Convert d to the degree type.
    const auto deg = ::obake::safe_cast<psk_deg_t<series_key_t<T>>>(d);

    // Create a temp vector of ints which we will use to
    // init the keys.
    ::std::vector<int> tmp(::obake::safe_cast<::std::vector<int>::size_type>(ss.size()));

    // Create the fw version of the symbol set.
    const detail::ss_fw ss_fw(ss);

    auto make_p_series = [&deg, &ss_fw, &ss, &tmp, &tss](const auto &n) {
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

        // Set the truncation.
        ::obake::set_truncation(retval, deg, tss);

        // Set back to zero the exponent that was previously set to 1.
        tmp[static_cast<::std::vector<int>::size_type>(ss.index_of(it))] = 0;

        return retval;
    };

    return detail::make_array(make_p_series(names)...);
}

} // namespace detail

// Power series creation functor, partial degree truncation.
template <typename T>
inline constexpr auto make_p_series_p
    = [](const auto &...args) OBAKE_SS_FORWARD_LAMBDA(detail::make_p_series_p_impl<T>(args...));

namespace power_series
{

// Specialisation of stream insertion in tex mode.
template <typename K, typename C>
inline void tex_stream_insert(::std::ostream &os, const p_series<K, C> &ps)
{
    // Stream the terms in tex mode.
    ::obake::detail::series_stream_terms_impl<true>(os, ps);

    // Add the truncation bits.
    using deg_t [[maybe_unused]] = decltype(::obake::degree(ps));
    os << ::std::visit(
        [](const auto &v) -> ::std::string {
            using type = remove_cvref_t<decltype(v)>;

            if constexpr (::std::is_same_v<type, detail::no_truncation>) {
                return "";
            } else if constexpr (tex_stream_insertable<const deg_t &>) {
                ::std::ostringstream oss(" + \\mathcal{O}\\left( ");
                oss.exceptions(::std::ios_base::failbit | ::std::ios_base::badbit);
                // I hope this does what it looks like it should be doing...
                oss.seekp(0, ::std::ios_base::end);

                if constexpr (::std::is_same_v<type, deg_t>) {
                    ::obake::tex_stream_insert(oss, v);
                } else {
                    ::obake::tex_stream_insert(oss, v.first);
                    oss << " ; ";

                    for (auto it = v.second.begin(); it != v.second.end();) {
                        oss << *it;

                        if (++it != v.second.end()) {
                            oss << ", ";
                        }
                    }
                }

                oss << " \\right)";

                return oss.str();
                // LCOV_EXCL_START
            } else {
                return " + ??";
            }
            // LCOV_EXCL_STOP
        },
        ::obake::get_truncation(ps));
}

namespace detail
{

// Algorithm selection for power series addsub.
template <bool AddOrSub, typename T, typename U>
constexpr int ps_addsub_algo()
{
    // Fetch the algo/ret type from the default addsub implementation
    // for series.
    constexpr auto p = ::obake::detail::series_default_addsub_algorithm_impl<AddOrSub, T, U>();

    if constexpr (p.first == 0) {
        // addsub not supported.
        return 0;
    } else {
        if constexpr (p.first < 3) {
            // The ranks of T and U differ. In this situation, one of the
            // operands becomes the coefficient of a term to be inserted
            // in the retval. If the retval
            // is a power series, we need to perform a truncation as the
            // newly-inserted term may need to be removed. If the retval
            // is not a power series, then it means that the power-series
            // operand is inserted as-is into the retval: no truncation needed,
            // and no point in invoking the addsub specialisation for power_series.
            return any_p_series<typename decltype(p.second)::type> ? 1 : 0;
        } else {
            static_assert(p.first == 3);

            // Both operands are series with the same rank, tag and key,
            // possibly different coefficients. Since, in this function,
            // we assume that at least one operand is a power series, then
            // the other operand is also a power series with the same rank.
            // We will then need to verify if the truncation matches
            // in the specialised addsub implementation.
            return 2;
        }
    }
}

// Implementation of addsub for power series.
template <bool AddOrSub, typename T, typename U>
inline auto ps_addsub_impl(T &&x, U &&y)
{
    constexpr auto algo = detail::ps_addsub_algo<AddOrSub, T &&, U &&>();

    if constexpr (algo == 1) {
        // The ranks of T and U differ, and the result is a power series.
        // The result will be copy/move inited from one of x or y, and it will
        // thus inherit the truncation policy/level. We need to run an explicit truncation
        // on the result because the newly-inserted term in ret may violate
        // the truncation settings.

        // NOTE: rather than doing a normal addsub and truncating later, perhaps
        // it would be slightly better for performance to avoid the insertion
        // altogether if it does not respect the truncation limit.

        auto ret = ::obake::detail::series_default_addsub_impl<AddOrSub>(::std::forward<T>(x), ::std::forward<U>(y));
        ::obake::truncate(ret);

        return ret;
    } else {
        static_assert(algo == 2);

        // T and U are both power series with the same rank/key, possibly
        // different coefficients.

        // Fetch the return type.
        using ret_t = decltype(
            ::obake::detail::series_default_addsub_impl<AddOrSub>(::std::forward<T>(x), ::std::forward<U>(y)));

        return ::std::visit(
            [&x, &y](const auto &v0, const auto &v1) -> ret_t {
                using type0 = remove_cvref_t<decltype(v0)>;
                using type1 = remove_cvref_t<decltype(v1)>;

                if constexpr (::std::is_same_v<type0, type1>) {
                    // The truncation policies match. In this case, we first
                    // check that the truncation levels also match, then
                    // we run a series addsub and finally assign the truncation
                    // level. No explicit truncation of the result is needed
                    // because we assume that x and y satisfy the truncation setting.
                    if (obake_unlikely(v0 != v1)) {
                        using namespace ::fmt::literals;

                        throw ::std::invalid_argument(
                            "Unable to {} two power series if their truncation levels do not match"_format(
                                AddOrSub ? "add" : "subtract"));
                    }

                    // Store the original tag.
                    auto orig_tag = x.tag();

                    // Perform the addsub.
                    auto ret = ::obake::detail::series_default_addsub_impl<AddOrSub>(::std::forward<T>(x),
                                                                                     ::std::forward<U>(y));

                    // Re-assign the original tag.
                    ret.tag() = ::std::move(orig_tag);

                    return ret;
                } else if constexpr (::std::is_same_v<
                                         type0,
                                         detail::no_truncation> || ::std::is_same_v<type1, detail::no_truncation>) {
                    // One series has no truncation, the other has some truncation. In this case, we
                    // run a series addsub, assign the truncation to the result
                    // and do an explicit truncation, as ret may contain terms that
                    // need to be discarded.
                    auto orig_tag = ::std::is_same_v<type1, detail::no_truncation> ? x.tag() : y.tag();
                    auto ret = ::obake::detail::series_default_addsub_impl<AddOrSub>(::std::forward<T>(x),
                                                                                     ::std::forward<U>(y));
                    ret.tag() = ::std::move(orig_tag);

                    ::obake::truncate(ret);

                    return ret;
                } else {
                    // The series have different truncation policies and both
                    // series are truncating.
                    using namespace ::fmt::literals;

                    throw ::std::invalid_argument(
                        "Unable to {} two power series if their truncation policies do not match"_format(
                            AddOrSub ? "add" : "subtract"));
                }
            },
            ::obake::get_truncation(x), ::obake::get_truncation(y));
    }
}

} // namespace detail

template <typename T, typename U>
    requires
    // At least one of the operands must be a power series.
    (any_p_series<remove_cvref_t<T>> || any_p_series<remove_cvref_t<U>>)
    // Check that the specialised addsub implementation
    // for power series is avaialable and appropriate.
    // Otherwise, the default series addsub will be used.
    && (detail::ps_addsub_algo<true, T &&, U &&>() != 0) inline
    // NOTE: don't use auto here due to this GCC bug:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95132
    ::obake::detail::series_default_addsub_ret_t<true, T &&, U &&> series_add(T &&x, U &&y)
{
    return detail::ps_addsub_impl<true>(::std::forward<T>(x), ::std::forward<U>(y));
}

template <typename T, typename U>
    requires(any_p_series<remove_cvref_t<T>> || any_p_series<remove_cvref_t<U>>)
    && (detail::ps_addsub_algo<false, T &&, U &&>()
        != 0) inline ::obake::detail::series_default_addsub_ret_t<false, T &&, U &&> series_sub(T &&x, U &&y)
{
    return detail::ps_addsub_impl<false>(::std::forward<T>(x), ::std::forward<U>(y));
}

namespace detail
{

// Algorithm selection for in-place power series addsub.
template <bool AddOrSub, typename T, typename U>
constexpr int ps_in_place_addsub_algo()
{
    // Fetch the algo/ret type from the default in-place addsub
    // implementation for series.
    constexpr auto p = ::obake::detail::series_default_in_place_addsub_algorithm_impl<AddOrSub, T, U>();

    if constexpr (p.first == 0) {
        // addsub not supported.
        return 0;
    } else {
        if constexpr (p.first == 1) {
            // T is a power series with a rank
            // less than U. In this case, the default series addsub
            // implementation delegates to the binary operator,
            // thus here we return 0 as there's no reason to use
            // a specialised primitive.
            return 0;
        } else if constexpr (p.first == 2) {
            // T is a power series with a rank greater
            // than U. In this case, U ends up being
            // inserted in T and we need to truncate.
            return 1;
        } else {
            static_assert(p.first == 3);

            // T and U are power series with same key type and rank,
            // possibly different coefficient type.
            // We will then need to verify if the truncation matches
            // in the specialised addsub implementation, and re-assign
            // the truncation level after the operation has been performed.
            return 2;
        }
    }
}

// Implementation of in-place addsub for power series.
template <bool AddOrSub, typename T, typename U>
inline decltype(auto) ps_in_place_addsub_impl(T &&x, U &&y)
{
    constexpr auto algo = detail::ps_in_place_addsub_algo<AddOrSub, T &&, U &&>();

    if constexpr (algo == 1) {
        // T is a power series with a rank greater
        // than U. In this case, U ends up being
        // inserted in T and we need to truncate.

        // NOTE: rather than doing a normal addsub and truncating later, perhaps
        // it would be slightly better for performance to avoid the insertion
        // altogether if it does not respect the truncation limit.

        decltype(auto) ret = ::obake::detail::series_default_in_place_addsub_impl<AddOrSub>(::std::forward<T>(x),
                                                                                            ::std::forward<U>(y));

        ::obake::truncate(x);

        return ret;
    } else {
        static_assert(algo == 2);

        // T and U are both power series with the same rank/key, possibly
        // different coefficients.

        // Fetch the return type.
        using ret_t = decltype(
            ::obake::detail::series_default_in_place_addsub_impl<AddOrSub>(::std::forward<T>(x), ::std::forward<U>(y)));

        return ::std::visit(
            [&x, &y](const auto &v0, const auto &v1) -> ret_t {
                using type0 = remove_cvref_t<decltype(v0)>;
                using type1 = remove_cvref_t<decltype(v1)>;

                if constexpr (::std::is_same_v<type0, type1>) {
                    // The truncation policies match. In this case, we first
                    // check that the truncation levels also match, then
                    // we run a series in-place addsub and finally assign the truncation
                    // level. No explicit truncation of the result is needed
                    // because we assume that x and y satisfy the truncation setting.
                    if (obake_unlikely(v0 != v1)) {
                        using namespace ::fmt::literals;

                        throw ::std::invalid_argument(
                            "Unable to {} two power series in place if "
                            "their truncation levels do not match"_format(AddOrSub ? "add" : "subtract"));
                    }

                    // Store the original tag.
                    auto orig_tag = x.tag();

                    // Perform the addsub.
                    decltype(auto) ret = ::obake::detail::series_default_in_place_addsub_impl<AddOrSub>(
                        ::std::forward<T>(x), ::std::forward<U>(y));

                    // NOTE: the tag re-assign is needed because the retval may have
                    // been reconstructed from scratch if symbol set extension was required.
                    x.tag() = ::std::move(orig_tag);

                    return ret;
                } else if constexpr (::std::is_same_v<
                                         type0,
                                         detail::no_truncation> || ::std::is_same_v<type1, detail::no_truncation>) {
                    // One series has no truncation, the other has some truncation. In this case, we
                    // run a series in-place addsub, assign the truncation to the result
                    // and do an explicit truncation, as ret may contain terms that
                    // need to be discarded.
                    auto orig_tag = ::std::is_same_v<type1, detail::no_truncation> ? x.tag() : y.tag();
                    decltype(auto) ret = ::obake::detail::series_default_in_place_addsub_impl<AddOrSub>(
                        ::std::forward<T>(x), ::std::forward<U>(y));
                    ret.tag() = ::std::move(orig_tag);

                    ::obake::truncate(ret);

                    return ret;
                } else {
                    using namespace ::fmt::literals;

                    throw ::std::invalid_argument(
                        "Unable to {} two power series in place if their truncation policies do not match"_format(
                            AddOrSub ? "add" : "subtract"));
                }
            },
            ::obake::get_truncation(x), ::obake::get_truncation(y));
    }
}

} // namespace detail

template <typename T, typename U>
    requires
    // T must be a power series.
    // NOTE: if T is not a power series and U is,
    // then there are the following possibilities:
    // - rank_T < rank_U -> this delegates to the binary
    //   operator, does not need special casing,
    // - rank_T > rank_U -> this inserts U into T, which
    //   is not a power series, no special casing needed.
    // - rank_T == rank_U -> this is not possible because
    //   T is not a power series, thus its tag differs from U's
    //   which disables the default implementation.
    any_p_series<remove_cvref_t<T>>
    // Check that the specialised addsub implementation
    // for power series is avaialable and appropriate.
    // Otherwise, the default series addsub will be used.
    && (detail::ps_in_place_addsub_algo<true, T &&, U &&>() != 0) inline remove_cvref_t<T> &series_in_place_add(T &&x,
                                                                                                                U &&y)
{
    return detail::ps_in_place_addsub_impl<true>(::std::forward<T>(x), ::std::forward<U>(y));
}

template <typename T, typename U>
    requires any_p_series<remove_cvref_t<
        T>> && (detail::ps_in_place_addsub_algo<false, T &&, U &&>() != 0) inline remove_cvref_t<T> &series_in_place_sub(T &&x, U &&y)
{
    return detail::ps_in_place_addsub_impl<false>(::std::forward<T>(x), ::std::forward<U>(y));
}

namespace detail
{

// Algorithm selection for power series mul.
template <typename T, typename U>
constexpr bool ps_mul_algo()
{
    // Fetch the (partial) degree type.
    using deg_t = decltype(::obake::degree(::std::declval<const T &>()));

    // NOTE: the truncated multiplication algorithm selection also checks for the
    // availability of untruncated multiplication.
    if constexpr (polynomials::detail::poly_mul_truncated_degree_algo<T, U, deg_t> != 0
                  && polynomials::detail::poly_mul_truncated_p_degree_algo<T, U, deg_t> != 0) {
        // The only thing we need to check is that the result of the poly mul
        // is still a power series. This is necessary because poly_mul_ret_t constructs
        // a series type from the original key/tag but with the coefficient determined
        // by the meta-programming algorithm. If such coefficient is not suitable for use
        // in a power series, we must disable the multiplication.
        using ret_t = ::obake::polynomials::detail::poly_mul_ret_t<T, U>;

        return any_p_series<ret_t>;
    } else {
        return 0;
    }
}

} // namespace detail

// Multiplication between two power series with the same rank via (truncated) polynomial multiplication.
// NOTE: for the other multiplication cases (i.e., those relying on series' default mul implementation)
// we don't require explicit truncation and we ensure that the tag is preserved correctly.
template <typename K, typename C0, typename C1>
requires(detail::ps_mul_algo<p_series<K, C0>, p_series<K, C1>>() == true) inline ::obake::polynomials::detail::
    poly_mul_ret_t<p_series<K, C0>, p_series<K, C1>> series_mul(const p_series<K, C0> &ps0, const p_series<K, C1> &ps1)
{
    // Fetch the return type.
    using ret_t = ::obake::polynomials::detail::poly_mul_ret_t<p_series<K, C0>, p_series<K, C1>>;

    // Fetch the (partial) degree type.
    using deg_t [[maybe_unused]] = decltype(::obake::degree(ps0));

    return ::std::visit(
        [&ps0, &ps1](const auto &v0, const auto &v1) -> ret_t {
            using type0 = remove_cvref_t<decltype(v0)>;
            using type1 = remove_cvref_t<decltype(v1)>;

            if constexpr (::std::is_same_v<type0, type1>) {
                // The truncation policies match. In this case, we first
                // check that the truncation levels also match, then
                // we run the truncated multiplication. We will have
                // to assign the tag to the return value.
                if (obake_unlikely(v0 != v1)) {
                    throw ::std::invalid_argument(
                        "Unable to multiply two power series if their truncation levels do not match");
                }

                if constexpr (::std::is_same_v<type0, detail::no_truncation>) {
                    // Untruncated multiplication.
                    return polynomials::detail::poly_mul_impl_switch(ps0, ps1);
                } else {
                    // Store the original tag.
                    auto orig_tag = ps0.tag();

                    if constexpr (::std::is_same_v<type0, deg_t>) {
                        // Total degree truncation.
                        auto ret = polynomials::detail::poly_mul_impl_switch(ps0, ps1, v0);
                        ret.tag() = ::std::move(orig_tag);
                        return ret;
                    } else {
                        // Partial degree truncation.
                        auto ret = polynomials::detail::poly_mul_impl_switch(ps0, ps1, v0.first, v0.second);
                        ret.tag() = ::std::move(orig_tag);
                        return ret;
                    }
                }
            } else if constexpr (::std::is_same_v<type0, detail::no_truncation>) {
                // ps0 has no truncation, ps1 has truncation. Run the truncated multiplication
                // and assign ps1's tag to the retval.
                auto orig_tag = ps1.tag();

                if constexpr (::std::is_same_v<type1, deg_t>) {
                    // Total degree truncation.
                    auto ret = polynomials::detail::poly_mul_impl_switch(ps0, ps1, v1);
                    ret.tag() = ::std::move(orig_tag);
                    return ret;
                } else {
                    // Partial degree truncation.
                    auto ret = polynomials::detail::poly_mul_impl_switch(ps0, ps1, v1.first, v1.second);
                    ret.tag() = ::std::move(orig_tag);
                    return ret;
                }
            } else if constexpr (::std::is_same_v<type1, detail::no_truncation>) {
                // ps0 has truncation, ps1 has no truncation. Run the truncated multiplication
                // and assign ps0's tag to the retval.
                auto orig_tag = ps0.tag();

                if constexpr (::std::is_same_v<type0, deg_t>) {
                    // Total degree truncation.
                    auto ret = polynomials::detail::poly_mul_impl_switch(ps0, ps1, v0);
                    ret.tag() = ::std::move(orig_tag);
                    return ret;
                } else {
                    // Partial degree truncation.
                    auto ret = polynomials::detail::poly_mul_impl_switch(ps0, ps1, v0.first, v0.second);
                    ret.tag() = ::std::move(orig_tag);
                    return ret;
                }
            } else {
                throw ::std::invalid_argument(
                    "Unable to multiply two power series if their truncation policies do not match");
            }
        },
        ::obake::get_truncation(ps0), ::obake::get_truncation(ps1));
}

// Exponentiation: we re-use the poly implementation, ensuring
// that the output is properly truncated.
template <typename T, typename U>
    requires any_p_series<remove_cvref_t<
        T>> && (customisation::internal::series_default_pow_algo<T &&, U &&> != 0) inline customisation::internal::series_default_pow_ret_t<T &&, U &&> pow(T &&x, U &&y)
{
    // Store x's tag.
    auto orig_tag = x.tag();

    // Perform the operation.
    auto ret = polynomials::detail::pow_poly_impl(::std::forward<T>(x), ::std::forward<U>(y));

    // Re-assign the tag and truncate.
    ret.tag() = ::std::move(orig_tag);
    ::obake::truncate(ret);

    return ret;
}

// Substitution.
// NOTE: we will be using poly's implementation, which
// is currently based on arithmetic operations and which
// should then work reasonably well wrt respecting and
// propagating the truncation settings. There are however
// corner cases in which the retval may be untruncated
// even if the input object(s) have truncation (e.g.,
// empty x). Not sure what's the best way of dealing with this.
template <typename T, typename U>
    requires any_p_series<remove_cvref_t<
        T>> && (polynomials::detail::poly_subs_algo<T &&, U> != 0) inline polynomials::detail::poly_subs_ret_t<T &&, U> subs(T &&x, const symbol_map<U> &sm)
{
    return polynomials::detail::poly_subs_impl(::std::forward<T>(x), sm);
}

// Diff.
// NOTE: we will be using poly's implementation, which
// is currently based on arithmetic operations or term insertions.
// In the latter case, differentiation should never increase the degree
// and thus no truncation is needed. Note that if diff is not done
// via term insertion, there may be corner cases in which the return
// value is not truncated even if x is. If this becomes a problem,
// we can enable this specialisation only for some values of diff_algo.
template <typename T>
    requires any_p_series<remove_cvref_t<
        T>> && (polynomials::detail::poly_diff_algo<T &&> != 0) inline polynomials::detail::poly_diff_ret_t<T &&> diff(T &&x, const ::std::string &s)
{
    return polynomials::detail::poly_diff_impl(::std::forward<T>(x), s);
}

// Integrate.
// NOTE: we will be using poly's implementation, which
// is currently based on arithmetic operations and term insertions.
// In the latter case, we need explicit truncation because integration
// may end up increasing the degree. Note that if integrate is not done
// via term insertion, there may be corner cases in which the return
// value is not truncated even if x is. If this becomes a problem,
// we can enable this specialisation only for some values of integrate_algo.
template <typename T>
    requires any_p_series<remove_cvref_t<
        T>> && (polynomials::detail::poly_integrate_algo<T &&> != 0) inline polynomials::detail::poly_integrate_ret_t<T &&> integrate(T &&x, const ::std::string &s)
{
    auto ret = polynomials::detail::poly_integrate_impl(::std::forward<T>(x), s);

    if constexpr (::std::is_same_v<decltype(ret), remove_cvref_t<T>>) {
        // NOTE: if the return type matches the input type,
        // then the result was calculated via repeated term insertions
        // and we need to explicitly truncate.
        ::obake::truncate(ret);
    }

    return ret;
}

} // namespace power_series

} // namespace obake

#endif
