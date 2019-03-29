// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_RANGES_HPP
#define PIRANHA_RANGES_HPP

#include <iterator>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace detail
{

namespace begin_using_adl
{

using ::std::begin;

template <typename T>
constexpr auto call_begin(T &&x) PIRANHA_SS_FORWARD_FUNCTION(begin(::std::forward<T>(x)));

template <typename T>
using begin_t = decltype(::piranha::detail::begin_using_adl::call_begin(::std::declval<T>()));

} // namespace begin_using_adl

namespace end_using_adl
{

using ::std::end;

template <typename T>
constexpr auto call_end(T &&x) PIRANHA_SS_FORWARD_FUNCTION(end(::std::forward<T>(x)));

template <typename T>
using end_t = decltype(::piranha::detail::end_using_adl::call_end(::std::declval<T>()));

} // namespace end_using_adl

#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T>
requires Iterator<begin_using_adl::begin_t<T>>
#else
template <typename T, ::std::enable_if_t<is_iterator_v<detected_t<begin_using_adl::begin_t, T>>, int> = 0>
#endif
    constexpr auto begin_impl(T &&x) PIRANHA_SS_FORWARD_FUNCTION(begin_using_adl::call_begin(::std::forward<T>(x)));

#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T>
requires Iterator<end_using_adl::end_t<T>>
#else
template <typename T, ::std::enable_if_t<is_iterator_v<detected_t<end_using_adl::end_t, T>>, int> = 0>
#endif
    constexpr auto end_impl(T &&x) PIRANHA_SS_FORWARD_FUNCTION(end_using_adl::call_end(::std::forward<T>(x)));

} // namespace detail

inline constexpr auto begin =
    [](auto &&x) PIRANHA_SS_FORWARD_LAMBDA(detail::begin_impl(::std::forward<decltype(x)>(x)));

inline constexpr auto end = [](auto &&x) PIRANHA_SS_FORWARD_LAMBDA(detail::end_impl(::std::forward<decltype(x)>(x)));

namespace detail
{

template <typename T>
using begin_t = decltype(::piranha::begin(::std::declval<T>()));

template <typename T>
using end_t = decltype(::piranha::end(::std::declval<T>()));

} // namespace detail

template <typename T>
using is_range = ::std::conjunction<is_detected<detail::begin_t, T>, is_detected<detail::end_t, T>,
                                    ::std::is_same<detected_t<detail::begin_t, T>, detected_t<detail::end_t, T>>>;

template <typename T>
inline constexpr bool is_range_v = is_range<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Range = requires(T &&x)
{
    requires Same<decltype(::piranha::begin(::std::forward<T>(x))), decltype(::piranha::end(::std::forward<T>(x)))>;
};

#endif

} // namespace piranha

#endif
