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

#include <piranha/config.hpp>
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

} // namespace polynomials

template <typename C, typename K>
using polynomial = polynomials::polynomial<C, K>;

} // namespace piranha

#endif
