// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_MONOMIAL_POW_HPP
#define PIRANHA_POLYNOMIALS_MONOMIAL_POW_HPP

#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/not_implemented.hpp>
#include <piranha/detail/priority_tag.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace customisation
{

// External customisation point for piranha::monomial_pow().
template <typename T, typename U
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto monomial_pow = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto monomial_pow_impl(T &&x, U &&y, const symbol_set &ss, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::monomial_pow<T &&, U &&>)(::std::forward<T>(x), ::std::forward<U>(y),
                                                                          ss));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto monomial_pow_impl(T &&x, U &&y, const symbol_set &ss, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(monomial_pow(::std::forward<T>(x), ::std::forward<U>(y), ss));

// Helper to enable the implementation only
// if the return type is T (after cvref removal).
template <typename T, typename U>
using monomial_pow_impl_ret_t = decltype(detail::monomial_pow_impl(
    ::std::declval<T>(), ::std::declval<U>(), ::std::declval<const symbol_set &>(), priority_tag<1>{}));

template <typename T, typename U,
          ::std::enable_if_t<::std::is_same_v<detected_t<monomial_pow_impl_ret_t, T, U>, remove_cvref_t<T>>, int> = 0>
constexpr auto monomial_pow_impl_with_ret_check(T &&x, U &&y, const symbol_set &ss)
    PIRANHA_SS_FORWARD_FUNCTION(detail::monomial_pow_impl(::std::forward<T>(x), ::std::forward<U>(y), ss,
                                                          priority_tag<1>{}));

} // namespace detail

#if defined(_MSC_VER)

struct monomial_pow_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y, const symbol_set &ss) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(detail::monomial_pow_impl_with_ret_check(::std::forward<T>(x),
                                                                                    ::std::forward<U>(y), ss))
};

inline constexpr auto monomial_pow = monomial_pow_msvc{};

#else

inline constexpr auto monomial_pow = [](auto &&x, auto &&y, const symbol_set &ss) PIRANHA_SS_FORWARD_LAMBDA(
    detail::monomial_pow_impl_with_ret_check(::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), ss));

#endif

namespace detail
{

template <typename T, typename U>
using monomial_pow_t
    = decltype(::piranha::monomial_pow(::std::declval<T>(), ::std::declval<U>(), ::std::declval<const symbol_set &>()));

}

// NOTE: runtime requirement: the result of the exponentiation
// must be compatible with the reference symbol set.
template <typename T, typename U>
using is_exponentiable_monomial = is_detected<detail::monomial_pow_t, T, U>;

template <typename T, typename U>
inline constexpr bool is_exponentiable_monomial_v = is_exponentiable_monomial<T, U>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
PIRANHA_CONCEPT_DECL ExponentiableMonomial = requires(T &&x, U &&y, const symbol_set &ss)
{
    ::piranha::monomial_pow(::std::forward<T>(x), ::std::forward<U>(y), ss);
};

#endif

} // namespace piranha

#endif
