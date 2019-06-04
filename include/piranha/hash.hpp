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
#include <string>
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
// NOTE: MSVC 2015 and earlier don't have a poisoned std::hash:
// if it is instantiated with an unsupported type, it will error
// out with a static assert instead of sfinaeing. As a compromise,
// enable this implementation only for a few known types.
#if defined(_MSC_VER) && _MSC_VER < 1910

template <typename T, ::std::enable_if_t<::std::disjunction_v<::std::is_arithmetic<remove_cvref_t<T>>,
                                                              ::std::is_same<::std::string, remove_cvref_t<T>>>,
                                         int> = 0>
constexpr ::std::size_t hash_impl(T &&x, priority_tag<0>)
{
    return ::std::hash<remove_cvref_t<T>>{}(::std::forward<T>(x));
}

#else

template <typename T>
constexpr auto hash_impl(T &&x, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(::std::hash<remove_cvref_t<T>>{}(::std::forward<T>(x)));

#endif

// Machinery to enable the hash implementation only if the return
// type is std::size_t.
template <typename T>
using hash_impl_ret_t = decltype(detail::hash_impl(::std::declval<T>(), priority_tag<2>{}));

template <typename T, ::std::enable_if_t<::std::is_same_v<detected_t<hash_impl_ret_t, T>, ::std::size_t>, int> = 0>
constexpr auto hash_impl_with_ret_check(T &&x)
    PIRANHA_SS_FORWARD_FUNCTION(detail::hash_impl(::std::forward<T>(x), priority_tag<2>{}));

} // namespace detail

inline constexpr auto hash =
    [](auto &&x) PIRANHA_SS_FORWARD_LAMBDA(detail::hash_impl_with_ret_check(::std::forward<decltype(x)>(x)));

namespace detail
{

template <typename T>
using hash_t = decltype(::piranha::hash(::std::declval<T>()));

}

template <typename T>
using is_hashable = is_detected<detail::hash_t, T>;

template <typename T>
inline constexpr bool is_hashable_v = is_hashable<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Hashable = requires(T &&x)
{
    ::piranha::hash(::std::forward<T>(x));
};

#endif

} // namespace piranha

#endif
