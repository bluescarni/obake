// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_MATH_DEGREE_HPP
#define PIRANHA_MATH_DEGREE_HPP

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

// External customisation point for piranha::degree().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto degree = not_implemented;

namespace internal
{

// Internal customisation point for piranha::degree().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
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
    PIRANHA_SS_FORWARD_FUNCTION((customisation::degree<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto degree_impl(T &&x, priority_tag<1>) PIRANHA_SS_FORWARD_FUNCTION(degree(::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto degree_impl(T &&x, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::internal::degree<T &&>)(::std::forward<T>(x)));

} // namespace detail

#if defined(_MSC_VER)

struct degree_msvc {
    template <typename T>
    constexpr auto operator()(T &&x) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(detail::degree_impl(::std::forward<T>(x), detail::priority_tag<2>{}))
};

inline constexpr auto degree = degree_msvc{};

#else

inline constexpr auto degree = [](auto &&x)
    PIRANHA_SS_FORWARD_LAMBDA(detail::degree_impl(::std::forward<decltype(x)>(x), detail::priority_tag<2>{}));

#endif

namespace detail
{

template <typename T>
using degree_t = decltype(::piranha::degree(::std::declval<T>()));

}

template <typename T>
using is_with_degree = is_detected<detail::degree_t, T>;

template <typename T>
inline constexpr bool is_with_degree_v = is_with_degree<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL WithDegree = requires(T &&x)
{
    ::piranha::degree(::std::forward<T>(x));
};

#endif

} // namespace piranha

#endif
