// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_MATH_IS_ZERO_HPP
#define PIRANHA_MATH_IS_ZERO_HPP

#include <utility>

#include <mp++/config.hpp>

#if defined(MPPP_WITH_MPFR)

// NOTE: mppp::real does not have an is_zero() overload, thus
// we must provide here an implementation.
#include <mp++/real.hpp>

#endif

#include <piranha/config.hpp>
#include <piranha/detail/not_implemented.hpp>
#include <piranha/detail/priority_tag.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace customisation
{

// External customisation point for piranha::is_zero().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto is_zero = not_implemented;

namespace internal
{

// Internal customisation point for piranha::is_zero().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto is_zero = not_implemented;

} // namespace internal

} // namespace customisation

namespace detail
{

#if defined(MPPP_WITH_MPFR)

// Implementation of is_zero() for mppp::real. It will be found
// by the unqualified function call overload of is_zero_impl().
inline bool is_zero(const mppp::real &r)
{
    return r.zero_p();
}

#endif

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto is_zero_impl(T &&x, priority_tag<3>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::is_zero<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto is_zero_impl(T &&x, priority_tag<2>) PIRANHA_SS_FORWARD_FUNCTION(is_zero(::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto is_zero_impl(T &&x, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::internal::is_zero<T &&>)(::std::forward<T>(x)));

// Lowest-priority: implementation based on the comparison operator and construction from the zero
// integral constant.
// NOTE: this must go into lowest priority, we want the ADL-based implementation to have
// the precedence.
template <typename T>
constexpr auto is_zero_impl(T &&x, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(::std::forward<T>(x) == remove_cvref_t<T>(0));

} // namespace detail

// NOTE: forcibly cast to bool the return value, so that if the selected implementation
// returns a type which is not convertible to bool, this call will SFINAE out.
inline constexpr auto is_zero = [](auto &&x) PIRANHA_SS_FORWARD_LAMBDA(
    static_cast<bool>(detail::is_zero_impl(::std::forward<decltype(x)>(x), detail::priority_tag<3>{})));

namespace detail
{

template <typename T>
using is_zero_t = decltype(::piranha::is_zero(::std::declval<T>()));

}

template <typename T>
using is_zero_testable = is_detected<detail::is_zero_t, T>;

template <typename T>
inline constexpr bool is_zero_testable_v = is_zero_testable<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL ZeroTestable = requires(T &&x)
{
    ::piranha::is_zero(::std::forward<T>(x));
};

#endif

} // namespace piranha

#endif
