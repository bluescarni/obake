// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POWER_SERIES_TRUNCATED_POWER_SERIES_HPP
#define OBAKE_POWER_SERIES_TRUNCATED_POWER_SERIES_HPP

#include <array>
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>

#include <obake/config.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/exceptions.hpp>
#include <obake/key/key_degree.hpp>
#include <obake/key/key_p_degree.hpp>
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

// Coefficient type for a TPS:
// - must satisfy is_cf,
// - must not be with degree
//   (via const lvalue).
template <typename C>
using is_tps_cf = ::std::conjunction<is_cf<C>, ::std::negation<is_with_degree<::std::add_lvalue_reference_t<const C>>>>;

template <typename C>
inline constexpr bool is_tps_cf_v = is_tps_cf<C>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename C>
OBAKE_CONCEPT_DECL TPSCf = is_tps_cf_v<C>;

#endif

// Key type for a TPS:
// - must satisfy is_key,
// - must have a semi-regular (partial) key degree
//   type (via const lvalue).
template <typename K>
using is_tps_key = ::std::conjunction<
    is_key<K>,
    // NOTE: nonesuch is not semi-regular, hence
    // is_semi_regular takes also care of the detection.
    is_semi_regular<detected_t<::obake::detail::key_degree_t, ::std::add_lvalue_reference_t<const K>>>,
    is_semi_regular<detected_t<::obake::detail::key_p_degree_t, ::std::add_lvalue_reference_t<const K>>>>;

template <typename K>
inline constexpr bool is_tps_key_v = is_tps_key<K>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename K>
OBAKE_CONCEPT_DECL TPSKey = is_tps_key_v<K>;

#endif

// Forward declaration.
#if defined(OBAKE_HAVE_CONCEPTS)
template <TPSKey K, TPSCf C>
#else
template <typename K, typename C, typename = ::std::enable_if_t<::std::conjunction_v<is_tps_key<K>, is_tps_cf<C>>>>
#endif
class truncated_power_series;

namespace detail
{

// Struct to represent the absence
// of truncation in a power series.
struct no_truncation {
};

// Implementation of the detection
// of the power series class.
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

// TODO update the type requirements
// so that truncate()'s implementation
// is guaranteed to work.
#if defined(OBAKE_HAVE_CONCEPTS)
template <TPSKey K, TPSCf C>
#else
template <typename K, typename C, typename>
#endif
class truncated_power_series
{
public:
    // Useful typedefs.
    using poly_t = polynomial<K, C>;
    using degree_t = ::obake::detail::key_degree_t<const K &>;
    using p_degree_t = ::obake::detail::key_p_degree_t<const K &>;
    // The truncation setting type.
    using trunc_t = ::boost::variant<detail::no_truncation, degree_t, ::std::tuple<p_degree_t, symbol_set>>;

    // Defaulted constructors/assignment operators.
    truncated_power_series() = default;
    truncated_power_series(const truncated_power_series &) = default;
    truncated_power_series(truncated_power_series &&) = default;
    truncated_power_series &operator=(const truncated_power_series &) = default;
    truncated_power_series &operator=(truncated_power_series &&) = default;

private:
    // Explicitly truncate the polynomial
    // to the current truncation level.
    void truncate()
    {
        struct truncate_visitor : ::boost::static_visitor<> {
            truncate_visitor(truncated_power_series &tps) : m_tps(tps) {}
            void operator()(const detail::no_truncation &) const {}
            void operator()(const degree_t &d) const
            {
                // NOTE: use specifically the implementations
                // in the polynomials namespace, which we know
                // use the machinery from series.hpp.
                ::obake::polynomials::truncate_degree(m_tps.m_poly, d);
            }
            void operator()(const ::std::tuple<p_degree_t, symbol_set> &t) const
            {
                ::obake::polynomials::truncate_p_degree(m_tps.m_poly, ::std::get<0>(t), ::std::get<1>(t));
            }
            truncated_power_series &m_tps;
        };

        ::boost::apply_visitor(truncate_visitor{*this}, m_trunc);
    }

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
        truncate();
    }
    // Constructor from generic object and total degree truncation
    // represented as a type safely castable to degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const U &l, ::std::false_type)
        : m_poly(::std::forward<T>(x)), m_trunc(::obake::safe_cast<degree_t>(l))
    {
        truncate();
    }
    // Constructor from generic object, symbol set and total degree truncation
    // represented as degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const symbol_set &ss, const U &l, ::std::true_type)
        : m_poly(::std::forward<T>(x), ss), m_trunc(::obake::safe_cast<degree_t>(l))
    {
        truncate();
    }
    // Constructor from generic object, symbol set and total degree truncation
    // represented as a type safely castable to degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const symbol_set &ss, const U &l, ::std::false_type)
        : m_poly(::std::forward<T>(x), ss), m_trunc(::obake::safe_cast<degree_t>(l))
    {
        truncate();
    }
    // Constructor from generic object and partial degree truncation
    // represented as p_degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const U &l, const symbol_set &s, ::std::true_type)
        : m_poly(::std::forward<T>(x)), m_trunc(::std::make_tuple(l, s))
    {
        truncate();
    }
    // Constructor from generic object and partial degree truncation
    // represented as a type safely castable to degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const U &l, const symbol_set &s, ::std::false_type)
        : m_poly(::std::forward<T>(x)), m_trunc(::std::make_tuple(::obake::safe_cast<p_degree_t>(l), s))
    {
        truncate();
    }
    // Constructor from generic object, symbol set and partial degree truncation
    // represented as p_degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const symbol_set &ss, const U &l, const symbol_set &s,
                                    ::std::true_type)
        : m_poly(::std::forward<T>(x), ss), m_trunc(::std::make_tuple(l, s))
    {
        truncate();
    }
    // Constructor from generic object, symbol set and partial degree truncation
    // represented as a type safely castable to degree_t.
    template <typename T, typename U>
    explicit truncated_power_series(ptag, T &&x, const symbol_set &ss, const U &l, const symbol_set &s,
                                    ::std::false_type)
        : m_poly(::std::forward<T>(x), ss), m_trunc(::std::make_tuple(::obake::safe_cast<p_degree_t>(l), s))
    {
        truncate();
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
    // Constructor from generic object and symbol set.
    // NOTE: this forwards to the series ctor from generic object
    // and symbol set, which is activated only if T is
    // of a lower rank than the poly rank (i.e., T must be rank 0).
    template <typename T, ::std::enable_if_t<::std::is_constructible_v<poly_t, T, const symbol_set &>, int> = 0>
    explicit truncated_power_series(T &&x, const symbol_set &ss) : m_poly(::std::forward<T>(x), ss)
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
    // Generic constructor with symbol set and total degree truncation.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <typename T, typename U>
    requires ::std::is_constructible_v<poly_t, T, const symbol_set &> && (SafelyCastable<const U &, degree_t> || ::std::is_same_v<U, degree_t>)
#else
    template <typename T, typename U,
              ::std::enable_if_t<::std::conjunction_v<::std::is_constructible<poly_t, T, const symbol_set &>,
                                                      ::std::disjunction<is_safely_castable<const U &, degree_t>,
                                                                         ::std::is_same<U, degree_t>>>,
                                 int> = 0>
#endif
        explicit truncated_power_series(T &&x, const symbol_set &ss, const U &l)
        : truncated_power_series(ptag{}, ::std::forward<T>(x), ss,l, ::std::is_same<U, degree_t>{})
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
    // Generic constructor with symbol set and partial degree truncation.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <typename T, typename U>
    requires ::std::is_constructible_v<poly_t, T, const symbol_set &> && (SafelyCastable<const U &, p_degree_t> || ::std::is_same_v<U, p_degree_t>)
#else
    template <typename T, typename U,
              ::std::enable_if_t<::std::conjunction_v<::std::is_constructible<poly_t, T, const symbol_set &>,
                                                      ::std::disjunction<is_safely_castable<const U &, p_degree_t>,
                                                                         ::std::is_same<U, p_degree_t>>>,
                                 int> = 0>
#endif
        explicit truncated_power_series(T &&x, const symbol_set &ss, const U &l, const symbol_set &s)
        : truncated_power_series(ptag{}, ::std::forward<T>(x), ss,l, s, ::std::is_same<U, p_degree_t>{})
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
            if constexpr (is_stream_insertable_v<const typename truncated_power_series<K, C>::degree_t &>) {
                m_os << d;
            } else {
                m_os << "<unprintable degree type>";
            }
        }
        void operator()(const ::std::tuple<typename tps_t::p_degree_t, symbol_set> &t) const
        {
            if constexpr (is_stream_insertable_v<const typename truncated_power_series<K, C>::p_degree_t &>) {
                m_os << ::std::get<0>(t);
            } else {
                m_os << "<unprintable partial degree type>";
            }
            m_os << ", " << ::obake::detail::to_string(::std::get<1>(t));
        }
        ::std::ostream &m_os;
    };
    ::boost::apply_visitor(trunc_stream_visitor{os}, tps._trunc());
    os << '\n';

    // Print the terms.
    ::obake::detail::series_stream_terms_impl<false>(os, tps._poly());

    return os;
}

// The swap primitive.
template <typename K, typename C>
inline void swap(truncated_power_series<K, C> &t1, truncated_power_series<K, C> &t2) noexcept
{
    using ::std::swap;
    swap(t1._poly(), t2._poly());
    swap(t1._trunc(), t2._trunc());
}

namespace detail
{

template <typename K, typename C>
inline auto tps_merge_trunc(const truncated_power_series<K, C> &t1, const truncated_power_series<K, C> &t2)
{
    const auto &tr1 = t1._trunc();
    const auto &tr2 = t2._trunc();

    const auto w1 = tr1.which();
    const auto w2 = tr1.which();

    if (obake_unlikely(w1 != w2 && w1 != 0 && w1 != 0)) {
        obake_throw(::std::invalid_argument, "");
    }

    if (w1 == 0) {
        return tr2;
    }

    if (w2 == 0) {
        return tr1;
    }
}

} // namespace detail

// NOTE: in the implementation, make sure we use the
// facilities from series.hpp for (partial) degree computation.
template <typename K, typename C>
inline auto degree(const truncated_power_series<K, C> &tps)
    OBAKE_SS_FORWARD_FUNCTION(::obake::customisation::internal::series_default_degree_impl{}(tps._poly()));

template <typename K, typename C>
inline auto p_degree(const truncated_power_series<K, C> &tps, const symbol_set &s)
    OBAKE_SS_FORWARD_FUNCTION(::obake::customisation::internal::series_default_p_degree_impl{}(tps._poly(), s));

} // namespace power_series

template <typename K, typename C>
using truncated_power_series = power_series::truncated_power_series<K, C>;

namespace detail
{

// Implementation details for the tps creation function from generators.

// This function will convert an array of polynomials into an array of tps
// using tps' constructors. The input arguments args will be passed as additional
// arguments to the constructors.
template <typename T, typename Poly, ::std::size_t N, ::std::size_t... Ns, typename... Args>
inline auto tps_poly_array_to_tps_impl(::std::array<Poly, N> &&a, ::std::index_sequence<Ns...>, const Args &... args)
{
    // NOTE: here we are constructing a tps from a polynomial.
    // The expression T(::std::get...) is a functional
    // cast expression:
    // https://en.cppreference.com/w/cpp/language/explicit_cast
    // Which is ultimately equivalent to a static_cast:
    // https://en.cppreference.com/w/cpp/language/static_cast
    // The static cast corresponds to a direct initialization:
    // https://en.cppreference.com/w/cpp/language/direct_initialization
    // In direct initialization, the constructor of T, if available,
    // has the precedence:
    // https://en.cppreference.com/w/cpp/language/overload_resolution
    // http://eel.is/c++draft/dcl.init
    // (see "Initialization by constructor" and "direct-initialization").
    // If the constructor of T is not available, then
    // user-defined conversions are considered as part of
    // the standard conversion sequence:
    // http://eel.is/c++draft/conv
    // http://eel.is/c++draft/class.conv
    // Explicit user-defined conversions are considered
    // (as well as implicit) because we are in a direct-initialization
    // context.
    //
    // With that in mind, GCC 7 warns here about the fact that the
    // conversion operator of polynomial to tps is also being
    // considered, but ultimately discarded in favour of tps'
    // constructor (as it should, according to my understanding).
    // The warning is not there in later GCC versions or clang
    // or MSVC, so I *think* this is a warning issue in this
    // specific GCC version.
#if defined(OBAKE_COMPILER_IS_GCC) && __GNUC__ == 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
    return ::std::array<T, N>{T(::std::get<Ns>(::std::move(a)), args...)...};
#if defined(OBAKE_COMPILER_IS_GCC) && __GNUC__ == 7
#pragma GCC diagnostic pop
#endif
}

template <typename T, typename Poly, ::std::size_t N, typename... Args>
inline auto tps_poly_array_to_tps(::std::array<Poly, N> &&a, const Args &... args)
{
    return detail::tps_poly_array_to_tps_impl<T>(::std::move(a), ::std::make_index_sequence<N>{}, args...);
}

// Generators only.
template <typename T, typename... Args,
          ::std::enable_if_t<::std::conjunction_v<power_series::detail::is_truncated_power_series_impl<T>,
                                                  make_polynomials_supported<typename T::poly_t, Args...>>,
                             int> = 0>
inline auto make_tps_impl(const Args &... args)
{
    return detail::tps_poly_array_to_tps<T>(::obake::make_polynomials<typename T::poly_t>(args...));
}

// Symbol set + generators.
template <typename T, typename... Args,
          ::std::enable_if_t<::std::conjunction_v<power_series::detail::is_truncated_power_series_impl<T>,
                                                  make_polynomials_supported<typename T::poly_t, Args...>>,
                             int> = 0>
inline auto make_tps_impl(const symbol_set &s, const Args &... args)
{
    return detail::tps_poly_array_to_tps<T>(::obake::make_polynomials<typename T::poly_t>(s, args...));
}

// Total degree truncation + generators.
template <
    typename T, typename U, typename... Args,
    ::std::enable_if_t<::std::conjunction_v<power_series::detail::is_truncated_power_series_impl<T>,
                                            make_polynomials_supported<typename T::poly_t, Args...>,
                                            ::std::disjunction<is_safely_castable<const U &, typename T::degree_t>,
                                                               ::std::is_same<U, typename T::degree_t>>>,
                       int> = 0>
inline auto make_tps_impl(const U &d, const Args &... args)
{
    return detail::tps_poly_array_to_tps<T>(::obake::make_polynomials<typename T::poly_t>(args...), d);
}

// Symbol set + total degree truncation + generators.
template <
    typename T, typename U, typename... Args,
    ::std::enable_if_t<::std::conjunction_v<power_series::detail::is_truncated_power_series_impl<T>,
                                            make_polynomials_supported<typename T::poly_t, Args...>,
                                            ::std::disjunction<is_safely_castable<const U &, typename T::degree_t>,
                                                               ::std::is_same<U, typename T::degree_t>>>,
                       int> = 0>
inline auto make_tps_impl(const symbol_set &s, const U &d, const Args &... args)
{
    return detail::tps_poly_array_to_tps<T>(::obake::make_polynomials<typename T::poly_t>(s, args...), d);
}

// Partial degree truncation + generators.
template <
    typename T, typename U, typename... Args,
    ::std::enable_if_t<::std::conjunction_v<power_series::detail::is_truncated_power_series_impl<T>,
                                            make_polynomials_supported<typename T::poly_t, Args...>,
                                            ::std::disjunction<is_safely_castable<const U &, typename T::p_degree_t>,
                                                               ::std::is_same<U, typename T::p_degree_t>>>,
                       int> = 0>
inline auto make_tps_impl(const U &d, const symbol_set &s, const Args &... args)
{
    return detail::tps_poly_array_to_tps<T>(::obake::make_polynomials<typename T::poly_t>(args...), d, s);
}

// Symbol set + partial degree truncation + generators.
template <
    typename T, typename U, typename... Args,
    ::std::enable_if_t<::std::conjunction_v<power_series::detail::is_truncated_power_series_impl<T>,
                                            make_polynomials_supported<typename T::poly_t, Args...>,
                                            ::std::disjunction<is_safely_castable<const U &, typename T::p_degree_t>,
                                                               ::std::is_same<U, typename T::p_degree_t>>>,
                       int> = 0>
inline auto make_tps_impl(const symbol_set &ss, const U &d, const symbol_set &s, const Args &... args)
{
    return detail::tps_poly_array_to_tps<T>(::obake::make_polynomials<typename T::poly_t>(ss, args...), d, s);
}

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

template <typename T>
struct make_truncated_power_series_msvc {
    template <typename... Args>
    constexpr auto operator()(const Args &... args) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::make_tps_impl<T>(args...))
};

template <typename T>
inline constexpr auto make_truncated_power_series = make_truncated_power_series_msvc<T>{};

#else

// tps creation functor.
template <typename T>
inline constexpr auto make_truncated_power_series
    = [](const auto &... args) OBAKE_SS_FORWARD_LAMBDA(detail::make_tps_impl<T>(args...));

#endif

} // namespace obake

#endif
