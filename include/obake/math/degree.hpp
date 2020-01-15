// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_DEGREE_HPP
#define OBAKE_MATH_DEGREE_HPP

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

// External customisation point for obake::degree().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto degree = not_implemented;

namespace internal
{

// Internal customisation point for obake::degree().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto degree = not_implemented;

} // namespace internal

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto degree_impl(T &&x, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::degree<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto degree_impl(T &&x, priority_tag<1>) OBAKE_SS_FORWARD_FUNCTION(degree(::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto degree_impl(T &&x, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::internal::degree<T &&>)(::std::forward<T>(x)));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct degree_msvc {
    template <typename T>
    constexpr auto operator()(T &&x) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::degree_impl(::std::forward<T>(x), detail::priority_tag<2>{}))
};

inline constexpr auto degree = degree_msvc{};

#else

inline constexpr auto degree = [](auto &&x)
    OBAKE_SS_FORWARD_LAMBDA(detail::degree_impl(::std::forward<decltype(x)>(x), detail::priority_tag<2>{}));

#endif

namespace detail
{

template <typename T>
using degree_t = decltype(::obake::degree(::std::declval<T>()));

}

template <typename T>
using is_with_degree = is_detected<detail::degree_t, T>;

template <typename T>
inline constexpr bool is_with_degree_v = is_with_degree<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL WithDegree = requires(T &&x)
{
    ::obake::degree(::std::forward<T>(x));
};

#endif

} // namespace obake

#endif
