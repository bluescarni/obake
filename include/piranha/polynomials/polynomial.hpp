// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP
#define PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP

#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/type_c.hpp>
#include <piranha/series.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace polynomials
{

struct tag {
};

template <typename K, typename C>
using polynomial = series<K, C, tag>;

namespace detail
{

template <typename T>
struct is_polynomial_impl : ::std::false_type {
};

template <typename K, typename C>
struct is_polynomial_impl<polynomial<K, C>> : ::std::true_type {
};

} // namespace detail

template <typename T>
using is_cvr_polynomial = detail::is_polynomial_impl<remove_cvref_t<T>>;

template <typename T>
inline constexpr bool is_cvr_polynomial_v = is_cvr_polynomial<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CvrPolynomial = is_cvr_polynomial_v<T>;

#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T, typename U>
requires CvrPolynomial<T> &&FloatingPoint<U>
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<is_cvr_polynomial<T>, ::std::is_floating_point<U>>, int> = 0>
#endif
    inline auto pow(const T &, const U &)
{
    return 1;
}

namespace detail
{

template <typename T, typename U>
constexpr auto poly_mul_algorithm_impl()
{
    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    constexpr auto rank_T = series_rank<rT>;
    constexpr auto rank_U = series_rank<rU>;

    // Shortcut for signalling that the mul implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, ::piranha::detail::type_c<void>{});

    if constexpr (::std::disjunction_v<::std::negation<is_cvr_polynomial<T>>, ::std::negation<is_cvr_polynomial<U>>>) {
        // T and U are not both polynomials.
        return failure;
    } else if constexpr (rank_T != rank_U) {
        // T and U are both polynomials, but with different rank.
        return failure;
    } else if constexpr (!::std::is_same_v<series_key_t<rT>, series_key_t<rU>>) {
        // The key types differ.
        return failure;
    } else {
        // T and U are both polynomials, same rank, same key.
        // Determine if we can multiply the coefficients, using
        // mutable rvalue or const lvalue refs.
        using ret_cf_t = detected_t<
            ::piranha::detail::mul_t,
            ::std::conditional_t<is_mutable_rvalue_reference_v<T>, series_cf_t<rT> &&, const series_cf_t<rT> &>,
            ::std::conditional_t<is_mutable_rvalue_reference_v<U>, series_cf_t<rU> &&, const series_cf_t<rU> &>>;

        if constexpr (is_cf_v<ret_cf_t>) {
            return ::std::make_pair(1, ::piranha::detail::type_c<polynomial<series_key_t<rT>, ret_cf_t>>{});
        } else {
            return failure;
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

template <typename T, typename U, ::std::enable_if_t<detail::poly_mul_algo<T &&, U &&> != 0, int> = 0>
constexpr detail::poly_mul_ret_t<T &&, U &&> series_mul(T &&, U &&)
{
    return detail::poly_mul_ret_t<T &&, U &&>{};
}

} // namespace polynomials

// Lift to the piranha namespace.
template <typename C, typename K>
using polynomial = polynomials::polynomial<C, K>;

} // namespace piranha

#endif
