// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POWER_SERIES_TRUNCATED_POWER_SERIES_HPP
#define OBAKE_POWER_SERIES_TRUNCATED_POWER_SERIES_HPP

#include <ostream>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>

#include <obake/config.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/math/degree.hpp>
#include <obake/math/p_degree.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_name.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace power_series
{

// Forward declaration.
template <typename, typename>
class truncated_power_series;

namespace detail
{

// Struct to represent the absence
// of truncation in a power series.
struct no_truncation {
};

template <typename T>
struct is_truncated_power_series_impl : ::std::false_type {
};

template <typename K, typename C>
struct is_truncated_power_series_impl<truncated_power_series<K, C>> : ::std::true_type {
};

} // namespace detail

template <typename T>
using is_cvr_truncated_power_series = detail::is_truncated_power_series_impl<remove_cvref_t<T>>;

template <typename T>
inline constexpr bool is_cvr_truncated_power_series_v = is_cvr_truncated_power_series<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL CvrTruncatedPowerSeries = is_cvr_truncated_power_series_v<T>;

#endif

namespace detail
{

// Machinery for tps' generic constructor.
template <typename T, typename K, typename C>
constexpr int tps_generic_ctor_algorithm_impl()
{
    // NOTE: check first if truncated_power_series<K, C> is a well-formed
    // type. Like this, if this function is instantiated with bogus
    // types, it will return 0 rather than giving a hard error.
    if constexpr (is_detected_v<truncated_power_series, K, C>) {
        using tps_t = truncated_power_series<K, C>;
        using rT = remove_cvref_t<T>;

        if constexpr (::std::is_same_v<rT, tps_t>) {
            // Avoid competition with the copy/move ctors.
            return 0;
        } else if constexpr (is_cvr_truncated_power_series_v<rT>) {
            // Construction from another tps. Make sure
            // the internal poly can be constructed from the
            // other internal poly.
            return ::std::is_constructible_v<typename tps_t::poly_t, decltype(::std::declval<T>()._poly())> ? 1 : 0;
        } else {
            // Construction from a non-tps object. Forward the
            // construction to the internal poly.
            return ::std::is_constructible_v<typename tps_t::poly_t, T> ? 2 : 0;
        }
    } else {
        return 0;
    }
}

template <typename T, typename K, typename C>
inline constexpr int tps_generic_ctor_algorithm = detail::tps_generic_ctor_algorithm_impl<T, K, C>();

} // namespace detail

template <typename T, typename K, typename C>
using is_tps_constructible = ::std::integral_constant<bool, detail::tps_generic_ctor_algorithm<T, K, C> != 0>;

template <typename T, typename K, typename C>
inline constexpr bool is_tps_constructible_v = is_tps_constructible<T, K, C>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T, typename K, typename C>
OBAKE_CONCEPT_DECL TPSConstructible = is_tps_constructible_v<T, K, C>;

#endif

// TODO type requirements for K and C:
// - must be suitable for use in polynomial,
// - poly_t must have total degree, and the degree
//   type must be suitable for use in variant,
// - same for the partial degree.
// TODO fix the requirements in the fwd declaration
// as well.
template <typename K, typename C>
class truncated_power_series
{
public:
    // Useful typedefs.
    using poly_t = polynomial<K, C>;
    using degree_t = decltype(::obake::degree(::std::declval<const poly_t &>()));
    using p_degree_t
        = decltype(::obake::p_degree(::std::declval<const poly_t &>(), ::std::declval<const symbol_set &>()));
    // The truncation setting type.
    using trunc_t = ::boost::variant<detail::no_truncation, degree_t, ::std::tuple<p_degree_t, symbol_set>>;

    // Defaulted constructors/assignment operators.
    truncated_power_series() = default;
    truncated_power_series(const truncated_power_series &) = default;
    truncated_power_series(truncated_power_series &&) = default;
    truncated_power_series &operator=(const truncated_power_series &) = default;
    truncated_power_series &operator=(truncated_power_series &&) = default;

private:
    // A tag structure for use in private ctors.
    struct ptag {
    };
    // Constructor from other tps.
    template <typename T>
    explicit truncated_power_series(ptag, T &&x, ::std::true_type)
        : m_poly(::std::forward<T>(x)._poly()), m_trunc(::std::forward<T>(x)._trunc())
    {
    }
    // Constructor from generic object (forwarding constructor to the internal polynomial).
    template <typename T>
    explicit truncated_power_series(ptag, T &&x, ::std::false_type) : m_poly(::std::forward<T>(x))
    {
    }
    // Constructor from generic object and total degree truncation
    // represented as degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const U &l, ::std::true_type)
        : m_poly(::std::forward<T>(x)), m_trunc(l)
    {
    }
    // Constructor from generic object and total degree truncation
    // represented as a type safely castable to degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const U &l, ::std::false_type)
        : m_poly(::std::forward<T>(x)), m_trunc(::obake::safe_cast<degree_t>(l))
    {
    }
    // Constructor from generic object and partial degree truncation
    // represented as p_degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const U &l, const symbol_set &s, ::std::true_type)
        : m_poly(::std::forward<T>(x)), m_trunc(::std::make_tuple(l, s))
    {
    }
    // Constructor from generic object and partial degree truncation
    // represented as a type safely castable to degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const U &l, const symbol_set &s, ::std::false_type)
        : m_poly(::std::forward<T>(x)), m_trunc(::std::make_tuple(::obake::safe_cast<p_degree_t>(l), s))
    {
    }

public:
    // Generic constructor.
    // NOTE: the generic construction algorithm must be well-documented,
    // as we rely on its behaviour in a variety of places.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <TPSConstructible<K, C> T>
#else
    template <typename T, ::std::enable_if_t<is_tps_constructible_v<T, K, C>, int> = 0>
#endif
    explicit truncated_power_series(T &&x)
        : truncated_power_series(ptag{}, ::std::forward<T>(x), is_cvr_truncated_power_series<T>{})
    {
    }
    // Generic constructor with total degree truncation.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <typename T, typename U>
    requires ::std::is_constructible_v<poly_t, T> && (SafelyCastable<const U &, degree_t> || ::std::is_same_v<U, degree_t>)
#else
    template <typename T, typename U,
              ::std::enable_if_t<::std::conjunction_v<::std::is_constructible<poly_t, T>,
                                                      ::std::disjunction<is_safely_castable<const U &, degree_t>,
                                                                         ::std::is_same<U, degree_t>>>,
                                 int> = 0>
#endif
        explicit truncated_power_series(T &&x, const U &l)
        : truncated_power_series(ptag{}, ::std::forward<T>(x), l, ::std::is_same<U, degree_t>{})
    {
    }
    // Generic constructor with partial degree truncation.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <typename T, typename U>
    requires ::std::is_constructible_v<poly_t, T> && (SafelyCastable<const U &, p_degree_t> || ::std::is_same_v<U, p_degree_t>)
#else
    template <typename T, typename U,
              ::std::enable_if_t<::std::conjunction_v<::std::is_constructible<poly_t, T>,
                                                      ::std::disjunction<is_safely_castable<const U &, p_degree_t>,
                                                                         ::std::is_same<U, p_degree_t>>>,
                                 int> = 0>
#endif
        explicit truncated_power_series(T &&x, const U &l, const symbol_set &s)
        : truncated_power_series(ptag{}, ::std::forward<T>(x), l, s, ::std::is_same<U, p_degree_t>{})
    {
    }
    // Generic assignment operator.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <TPSConstructible<K, C> T>
#else
    template <typename T, ::std::enable_if_t<is_tps_constructible_v<T, K, C>, int> = 0>
#endif
    truncated_power_series &operator=(T &&x)
    {
        return *this = truncated_power_series(::std::forward<T>(x));
    }

    // The polynomial getters.
    poly_t &_poly() &
    {
        return m_poly;
    }
    const poly_t &_poly() const &
    {
        return m_poly;
    }
    poly_t &&_poly() &&
    {
        return ::std::move(m_poly);
    }

    // The trunc getters.
    trunc_t &_trunc() &
    {
        return m_trunc;
    }
    const trunc_t &_trunc() const &
    {
        return m_trunc;
    }
    trunc_t &&_trunc() &&
    {
        return ::std::move(m_trunc);
    }

private:
    poly_t m_poly;
    trunc_t m_trunc;
};

template <typename K, typename C>
inline ::std::ostream &operator<<(::std::ostream &os, const truncated_power_series<K, C> &tps)
{
    using tps_t = truncated_power_series<K, C>;
    using poly_t = typename tps_t::poly_t;

    // Print the header.
    os << "Key type        : " << ::obake::type_name<series_key_t<poly_t>>() << '\n';
    os << "Coefficient type: " << ::obake::type_name<series_cf_t<poly_t>>() << '\n';
    os << "Rank            : " << series_rank<poly_t> << '\n';
    os << "Symbol set      : " << ::obake::detail::to_string(tps._poly().get_symbol_set()) << '\n';
    os << "Number of terms : " << tps._poly().size() << '\n';
    os << "Truncation      : ";
    struct trunc_stream_visitor : ::boost::static_visitor<> {
        trunc_stream_visitor(::std::ostream &s) : m_os(s) {}
        void operator()(const detail::no_truncation &) const
        {
            m_os << "None";
        }
        void operator()(const typename tps_t::degree_t &d) const
        {
            m_os << d;
        }
        void operator()(const ::std::tuple<typename tps_t::p_degree_t, symbol_set> &t) const
        {
            m_os << ::std::get<0>(t) << ", " << ::obake::detail::to_string(::std::get<1>(t));
        }
        ::std::ostream &m_os;
    };
    ::boost::apply_visitor(trunc_stream_visitor{os}, tps._trunc());
    os << '\n';

    // Print the terms.
    ::obake::detail::series_stream_terms_impl<false>(os, tps._poly());

    return os;
}

template <typename K, typename C>
inline auto degree(const truncated_power_series<K, C> &tps) OBAKE_SS_FORWARD_FUNCTION(::obake::degree(tps._poly()));

template <typename K, typename C>
inline auto p_degree(const truncated_power_series<K, C> &tps, const symbol_set &s)
    OBAKE_SS_FORWARD_FUNCTION(::obake::p_degree(tps._poly(), s));

} // namespace power_series

template <typename K, typename C>
using truncated_power_series = power_series::truncated_power_series<K, C>;

} // namespace obake

#endif
