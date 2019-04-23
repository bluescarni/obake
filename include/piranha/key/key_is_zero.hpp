// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_KEY_KEY_IS_ZERO_HPP
#define PIRANHA_KEY_KEY_IS_ZERO_HPP

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

// External customisation point for piranha::key_is_zero().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto key_is_zero = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto key_is_zero_impl(T &&x, const symbol_set &ss, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::key_is_zero<T &&>)(::std::forward<T>(x), ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto key_is_zero_impl(T &&x, const symbol_set &ss, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(key_is_zero(::std::forward<T>(x), ss));

} // namespace detail

// NOTE: forcibly cast to bool the return value, so that if the selected implementation
// returns a type which is not convertible to bool, this call will SFINAE out.
inline constexpr auto key_is_zero = [](auto &&x, const symbol_set &ss) PIRANHA_SS_FORWARD_LAMBDA(
    static_cast<bool>(detail::key_is_zero_impl(::std::forward<decltype(x)>(x), ss, detail::priority_tag<1>{})));

namespace detail
{

template <typename T>
using key_is_zero_t = decltype(::piranha::key_is_zero(::std::declval<T>(), ::std::declval<const symbol_set &>()));

}

template <typename T>
using is_zero_testable_key = is_detected<detail::key_is_zero_t, T>;

template <typename T>
inline constexpr bool is_zero_testable_key_v = is_zero_testable_key<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL ZeroTestableKey = requires(T &&x, const symbol_set &ss)
{
    ::piranha::key_is_zero(::std::forward<T>(x), ss);
};

#endif

} // namespace piranha

#endif
