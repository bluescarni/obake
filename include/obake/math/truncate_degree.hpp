// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_TRUNCATE_DEGREE_HPP
#define OBAKE_MATH_TRUNCATE_DEGREE_HPP

#include <type_traits>
#include <utility>

#include <obake/config.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::truncate_degree().
// NOTE: no internal customisation point for the time being,
// but it probably makes sense to have a default implementation
// for series kicking in when only the coefficient has a degree
// (i.e., for Poisson series).
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto truncate_degree = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto truncate_degree_impl(T &&x, U &&y, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::truncate_degree<T &&, U &&>)(::std::forward<T>(x), ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto truncate_degree_impl(T &&x, U &&y, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(truncate_degree(::std::forward<T>(x), ::std::forward<U>(y)));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct truncate_degree_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(void(detail::truncate_degree_impl(::std::forward<T>(x), ::std::forward<U>(y),
                                                                           detail::priority_tag<1>{})))
};

inline constexpr auto truncate_degree = truncate_degree_msvc{};

#else

inline constexpr auto truncate_degree =
    [](auto &&x, auto &&y) OBAKE_SS_FORWARD_LAMBDA(void(detail::truncate_degree_impl(
        ::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<1>{})));

#endif

namespace detail
{

template <typename T, typename U>
using truncate_degree_t = decltype(::obake::truncate_degree(::std::declval<T>(), ::std::declval<U>()));

}

template <typename T, typename U>
using is_degree_truncatable = is_detected<detail::truncate_degree_t, T, U>;

template <typename T, typename U>
inline constexpr bool is_degree_truncatable_v = is_degree_truncatable<T, U>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T, typename U>
OBAKE_CONCEPT_DECL DegreeTruncatable = requires(T &&x, U &&y)
{
    ::obake::truncate_degree(::std::forward<T>(x), ::std::forward<U>(y));
};

#endif

} // namespace obake

#endif
