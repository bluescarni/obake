// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP
#define PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/detail/type_c.hpp>
#include <piranha/key/key_merge_symbols.hpp>
#include <piranha/math/safe_cast.hpp>
#include <piranha/polynomials/monomial_mul.hpp>
#include <piranha/polynomials/monomial_range_overflow_check.hpp>
#include <piranha/ranges.hpp>
#include <piranha/series.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace polynomials
{

// The polynomial tag.
struct tag {
};

} // namespace polynomials

template <typename K, typename C>
using polynomial = series<K, C, polynomials::tag>;

namespace detail
{

template <typename T>
struct is_polynomial_impl : ::std::false_type {
};

template <typename K, typename C>
struct is_polynomial_impl<polynomial<K, C>> : ::std::true_type {
};

} // namespace detail

// Detect polynomials.
template <typename T>
using is_polynomial = detail::is_polynomial_impl<T>;

template <typename T>
inline constexpr bool is_polynomial_v = is_polynomial<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Polynomial = is_polynomial_v<T>;

#endif

namespace detail
{

// Enabler for make_polynomials():
// - T must be a polynomial,
// - std::string can be constructed from each input Args,
// - poly key can be constructed from a const int * range,
// - poly cf can be constructed from an integral literal.
template <typename T, typename... Args>
using make_polynomials_enabler
    = ::std::enable_if_t<::std::conjunction_v<is_polynomial<T>, ::std::is_constructible<::std::string, const Args &>...,
                                              ::std::is_constructible<series_key_t<T>, const int *, const int *>,
                                              ::std::is_constructible<series_cf_t<T>, int>>,
                         int>;

// Overload with a symbol set.
template <typename T, typename... Args, make_polynomials_enabler<T, Args...> = 0>
inline ::std::array<T, sizeof...(Args)> make_polynomials_impl(const symbol_set &ss, const Args &... names)
{
    [[maybe_unused]] auto make_poly = [&ss](const auto &n) {
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
        retval.set_symbol_set(ss);

        // Try to locate s within the symbol set.
        ::std::vector<int> tmp(::piranha::safe_cast<::std::vector<int>::size_type>(ss.size()));
        const auto it = ::std::lower_bound(ss.begin(), ss.end(), s);
        if (piranha_unlikely(it == ss.end() || *it != s)) {
            piranha_throw(::std::invalid_argument, "Cannot create a polynomial with symbol set " + detail::to_string(ss)
                                                       + " from the generator '" + s
                                                       + "': the generator is not in the symbol set");
        }

        // Set to 1 the exponent of the corresponding generator.
        tmp[static_cast<::std::vector<int>::size_type>(ss.index_of(it))] = 1;

        // Create and add a new term.
        retval.add_term(
            series_key_t<T>(static_cast<const int *>(tmp.data()), static_cast<const int *>(tmp.data() + tmp.size())),
            1);

        return retval;
    };

    return ::std::array<T, sizeof...(Args)>{make_poly(names)...};
}

// Overload without a symbol set.
template <typename T, typename... Args, make_polynomials_enabler<T, Args...> = 0>
inline ::std::array<T, sizeof...(Args)> make_polynomials_impl(const Args &... names)
{
    [[maybe_unused]] auto make_poly = [](const auto &n) {
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

    return ::std::array<T, sizeof...(Args)>{make_poly(names)...};
}

} // namespace detail

#if defined(_MSC_VER)

template <typename T>
struct make_polynomials_msvc {
    template <typename... Args>
    constexpr auto operator()(const Args &... args) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(detail::make_polynomials_impl<T>(args...))
};

template <typename T>
inline constexpr auto make_polynomials = make_polynomials_msvc<T>{};

#else

// Polynomial creation functor.
template <typename T>
inline constexpr auto make_polynomials
    = [](const auto &... args) PIRANHA_SS_FORWARD_LAMBDA(detail::make_polynomials_impl<T>(args...));

#endif

namespace polynomials
{

namespace detail
{

// Small helper to extract a const reference to
// a term's key (that is, the first element of the
// input pair p).
struct poly_term_key_ref_extractor {
    template <typename P>
    constexpr decltype(auto) operator()(const P &p) const noexcept
    {
        return static_cast<const typename P::first_type &>(p.first);
    }
};

// Meta-programming for selecting the algorithm and the return
// type of polynomial multiplication.
template <typename T, typename U>
constexpr auto poly_mul_algorithm_impl()
{
    // Shortcut for signalling that the mul implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, ::piranha::detail::type_c<void>{});

    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    if constexpr (::std::disjunction_v<::std::negation<is_polynomial<rT>>, ::std::negation<is_polynomial<rU>>>) {
        // T and U are not both polynomials.
        return failure;
    } else {
        constexpr auto rank_T = series_rank<rT>;
        constexpr auto rank_U = series_rank<rU>;

        if constexpr (rank_T != rank_U) {
            // T and U are both polynomials, but with different rank.
            return failure;
        } else if constexpr (!::std::is_same_v<series_key_t<rT>, series_key_t<rU>>) {
            // The key types differ.
            return failure;
        } else {
            // T and U are both polynomials, same rank, same key.
            // Determine if we can multiply the coefficients, using
            // const lvalue refs.
            using cf1_t = series_cf_t<rT>;
            using cf2_t = series_cf_t<rU>;
            using ret_cf_t = detected_t<::piranha::detail::mul_t, const cf1_t &, const cf2_t &>;

            if constexpr (is_cf_v<ret_cf_t>) {
                // The coefficient is valid.
                //
                // We need to perform monomial range checking on the input series.
                // Depending on runtime conditions, the check will be done on either
                // vector of copies of the original terms, or directly on the terms of the
                // series operands.

                // The vectors of copies of input terms.
                using xv_t = ::std::vector<::std::pair<series_key_t<rT>, cf1_t>>;
                using yv_t = ::std::vector<::std::pair<series_key_t<rU>, cf2_t>>;

                // The corresponding range types.
                using xr1_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<xv_t &>().cbegin(), poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<xv_t &>().cend(), poly_term_key_ref_extractor{})));
                using yr1_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<yv_t &>().cbegin(), poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<yv_t &>().cend(), poly_term_key_ref_extractor{})));

                // The original range types. We will be using const_iterator rvalues from the input series.
                using xr2_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<typename rT::const_iterator>(),
                                                     poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<typename rT::const_iterator>(),
                                                     poly_term_key_ref_extractor{})));
                using yr2_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<typename rU::const_iterator>(),
                                                     poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<typename rU::const_iterator>(),
                                                     poly_term_key_ref_extractor{})));

                if constexpr (::std::conjunction_v<
                                  are_overflow_testable_monomial_ranges<xr1_t, yr1_t>,
                                  are_overflow_testable_monomial_ranges<xr2_t, yr2_t>,
                                  // We may need to merge new symbols into the original key type.
                                  // NOTE: the key types of T and U must be identical at the moment,
                                  // so checking only T's key type is enough.
                                  // NOTE: the merging is done via a const ref.
                                  is_symbols_mergeable_key<const series_key_t<rT> &>,
                                  // Need to be able to multiply monomials. We work with lvalue
                                  // references, const for the arguments, mutable for the
                                  // return value.
                                  // NOTE: the key types of T and U must be identical at the moment,
                                  // so checking only T's key type is enough.
                                  is_multipliable_monomial<series_key_t<rT> &, const series_key_t<rT> &,
                                                           const series_key_t<rT> &>>) {
                    return ::std::make_pair(1, ::piranha::detail::type_c<polynomial<series_key_t<rT>, ret_cf_t>>{});
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
inline constexpr auto poly_mul_algorithm = detail::poly_mul_algorithm_impl<T, U>();

template <typename T, typename U>
inline constexpr int poly_mul_algo = poly_mul_algorithm<T, U>.first;

template <typename T, typename U>
using poly_mul_ret_t = typename decltype(poly_mul_algorithm<T, U>.second)::type;

} // namespace detail

} // namespace polynomials

} // namespace piranha

#endif
