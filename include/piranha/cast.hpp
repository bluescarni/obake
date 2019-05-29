// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_CAST_HPP
#define PIRANHA_CAST_HPP

#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/dependent_false.hpp>
#include <piranha/detail/not_implemented.hpp>
#include <piranha/detail/priority_tag.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace customisation
{

// External customisation point for piranha::cast().
template <typename To, typename From
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto cast = not_implemented;

} // namespace customisation

namespace detail
{

// NOTE: we need at least 1 declaration in order for the unqualified
// implementation below to work. This is because, in the unqualified
// call, we need to explicitly specify the first template parameter,
// and that makes it different from a "regular" function call. See
// here:
//
// https://stackoverflow.com/questions/44490086/adl-fails-or-not-done-for-function-with-additional-non-deduced-template-par
//
// Make sure this is always disabled.
template <typename, typename U, ::std::enable_if_t<dependent_false<U>::value, int> = 0>
void cast(U &&);

// Highest priority: explicit user override in the external customisation namespace.
template <typename To, typename From>
constexpr auto cast_impl(From &&x, priority_tag<2>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::cast<To, From &&>)(::std::forward<From>(x)));

// Unqualified function call implementation.
template <typename To, typename From>
constexpr auto cast_impl(From &&x, priority_tag<1>) PIRANHA_SS_FORWARD_FUNCTION(cast<To>(::std::forward<From>(x)));

// Lowest priority: implementation via static_cast.
template <typename To, typename From>
constexpr auto cast_impl(From &&x, priority_tag<0>) PIRANHA_SS_FORWARD_FUNCTION(To(::std::forward<From>(x)));

// Machinery to enable the cast implementation only if it
// actually returns To.
template <typename To, typename From>
using cast_impl_ret_t = decltype(detail::cast_impl<To>(::std::declval<From>(), priority_tag<2>{}));

template <typename To, typename From,
          ::std::enable_if_t<::std::is_same_v<detected_t<cast_impl_ret_t, To, From>, To>, int> = 0>
constexpr auto cast_impl_to_check(From &&x)
    PIRANHA_SS_FORWARD_FUNCTION(detail::cast_impl<To>(::std::forward<From>(x), priority_tag<2>{}));

} // namespace detail

template <typename To>
inline constexpr auto cast
    = [](auto &&x) PIRANHA_SS_FORWARD_LAMBDA(detail::cast_impl_to_check<To>(::std::forward<decltype(x)>(x)));

namespace detail
{

template <typename To, typename From>
using cast_t = decltype(::piranha::cast<To>(::std::declval<From>()));

}

template <typename From, typename To>
using is_castable = is_detected<detail::cast_t, To, From>;

template <typename From, typename To>
inline constexpr bool is_castable_v = is_castable<From, To>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename From, typename To>
PIRANHA_CONCEPT_DECL Castable = requires(From &&x)
{
    ::piranha::cast<To>(::std::forward<From>(x));
};

#endif

} // namespace piranha

#endif
