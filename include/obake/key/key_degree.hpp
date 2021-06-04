// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_KEY_KEY_DEGREE_HPP
#define OBAKE_KEY_KEY_DEGREE_HPP

#include <utility>

#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::key_degree().
template <typename T>
inline constexpr auto key_degree = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto key_degree_impl(T &&x, const symbol_set &ss, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::key_degree<T &&>)(::std::forward<T>(x), ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto key_degree_impl(T &&x, const symbol_set &ss, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(key_degree(::std::forward<T>(x), ss));

} // namespace detail

inline constexpr auto key_degree = [](auto &&x, const symbol_set &ss)
    OBAKE_SS_FORWARD_LAMBDA(detail::key_degree_impl(::std::forward<decltype(x)>(x), ss, detail::priority_tag<1>{}));

namespace detail
{

template <typename T>
using key_degree_t = decltype(::obake::key_degree(::std::declval<T>(), ::std::declval<const symbol_set &>()));

}

template <typename T>
using is_key_with_degree = is_detected<detail::key_degree_t, T>;

template <typename T>
inline constexpr bool is_key_with_degree_v = is_key_with_degree<T>::value;

template <typename T>
concept KeyWithDegree = requires(T &&x, const symbol_set &ss)
{
    ::obake::key_degree(::std::forward<T>(x), ss);
};

} // namespace obake

#endif
