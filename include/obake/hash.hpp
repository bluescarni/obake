// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_HASH_HPP
#define OBAKE_HASH_HPP

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::hash().
template <typename T>
inline constexpr auto hash = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto hash_impl(T &&x, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::hash<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto hash_impl(T &&x, priority_tag<1>) OBAKE_SS_FORWARD_FUNCTION(hash(::std::forward<T>(x)));

// Lowest priority: try to use std::hash.
template <typename T>
constexpr auto hash_impl(T &&x, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(::std::hash<::std::remove_cvref_t<T>>{}(::std::forward<T>(x)));

// Machinery to enable the hash implementation only if the return
// type is std::size_t.
template <typename T>
using hash_impl_ret_t = decltype(detail::hash_impl(::std::declval<T>(), priority_tag<2>{}));

template <typename T, ::std::enable_if_t<::std::is_same_v<detected_t<hash_impl_ret_t, T>, ::std::size_t>, int> = 0>
constexpr auto hash_impl_with_ret_check(T &&x)
    OBAKE_SS_FORWARD_FUNCTION(detail::hash_impl(::std::forward<T>(x), priority_tag<2>{}));

} // namespace detail

inline constexpr auto hash =
    [](auto &&x) OBAKE_SS_FORWARD_LAMBDA(detail::hash_impl_with_ret_check(::std::forward<decltype(x)>(x)));

namespace detail
{

template <typename T>
using hash_t = decltype(::obake::hash(::std::declval<T>()));

}

template <typename T>
using is_hashable = is_detected<detail::hash_t, T>;

template <typename T>
inline constexpr bool is_hashable_v = is_hashable<T>::value;

template <typename T>
concept Hashable = requires(T &&x)
{
    ::obake::hash(::std::forward<T>(x));
};

} // namespace obake

#endif
