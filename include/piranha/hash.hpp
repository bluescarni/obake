// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_HASH_HPP
#define PIRANHA_HASH_HPP

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/not_implemented.hpp>
#include <piranha/detail/priority_tag.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace customisation
{

// External customisation point for piranha::hash().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto hash = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto hash_impl(T &&x, priority_tag<2>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::hash<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto hash_impl(T &&x, priority_tag<1>) PIRANHA_SS_FORWARD_FUNCTION(hash(::std::forward<T>(x)));

// Lowest priority: try to use std::hash.
template <typename T>
constexpr auto hash_impl(T &&x, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(hash(::std::hash<remove_cvref_t<T>>{}(::std::forward<T>(x))));

// Machinery to enable the hash implementation only if the return
// type is std::size_t.
template <typename T>
using hash_impl_ret_t = decltype(::piranha::detail::hash_impl(::std::declval<T>(), priority_tag<2>{}));

template <typename T, ::std::enable_if_t<::std::is_same_v<detected_t<hash_impl_ret_t, T>, ::std::size_t>, int> = 0>
constexpr auto hash_impl_with_ret_check(T &&x)
    PIRANHA_SS_FORWARD_FUNCTION(::piranha::detail::hash_impl(::std::forward<T>(x), priority_tag<2>{}));

} // namespace detail

inline constexpr auto hash = [](auto &&x)
    PIRANHA_SS_FORWARD_LAMBDA(detail::hash_impl(::std::forward<decltype(x)>(x), detail::priority_tag<2>{}));

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
