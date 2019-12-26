// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_P_DEGREE_HPP
#define OBAKE_MATH_P_DEGREE_HPP

#include <utility>

#include <obake/config.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::p_degree().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto p_degree = not_implemented;

namespace internal
{

// Internal customisation point for obake::p_degree().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto p_degree = not_implemented;

} // namespace internal

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto p_degree_impl(T &&x, const symbol_set &ss, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::p_degree<T &&>)(::std::forward<T>(x), ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto p_degree_impl(T &&x, const symbol_set &ss, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(p_degree(::std::forward<T>(x), ss));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto p_degree_impl(T &&x, const symbol_set &ss, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::internal::p_degree<T &&>)(::std::forward<T>(x), ss));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct p_degree_msvc {
    template <typename T>
    constexpr auto operator()(T &&x, const symbol_set &ss) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::p_degree_impl(::std::forward<T>(x), ss, detail::priority_tag<2>{}))
};

inline constexpr auto p_degree = p_degree_msvc{};

#else

inline constexpr auto p_degree = [](auto &&x, const symbol_set &ss)
    OBAKE_SS_FORWARD_LAMBDA(detail::p_degree_impl(::std::forward<decltype(x)>(x), ss, detail::priority_tag<2>{}));

#endif

namespace detail
{

template <typename T>
using p_degree_t = decltype(::obake::p_degree(::std::declval<T>(), ::std::declval<const symbol_set &>()));

}

template <typename T>
using is_with_p_degree = is_detected<detail::p_degree_t, T>;

template <typename T>
inline constexpr bool is_with_p_degree_v = is_with_p_degree<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL WithPDegree = requires(T &&x, const symbol_set &ss)
{
    ::obake::p_degree(::std::forward<T>(x), ss);
};

#endif

} // namespace obake

#endif
