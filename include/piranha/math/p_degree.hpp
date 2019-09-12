// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_MATH_P_DEGREE_HPP
#define PIRANHA_MATH_P_DEGREE_HPP

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

// External customisation point for piranha::p_degree().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto p_degree = not_implemented;

namespace internal
{

// Internal customisation point for piranha::p_degree().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
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
    PIRANHA_SS_FORWARD_FUNCTION((customisation::p_degree<T &&>)(::std::forward<T>(x), ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto p_degree_impl(T &&x, const symbol_set &ss, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION(p_degree(::std::forward<T>(x), ss));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto p_degree_impl(T &&x, const symbol_set &ss, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::internal::p_degree<T &&>)(::std::forward<T>(x), ss));

} // namespace detail

#if defined(_MSC_VER)

struct p_degree_msvc {
    template <typename T>
    constexpr auto operator()(T &&x, const symbol_set &ss) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(detail::p_degree_impl(::std::forward<T>(x), ss, detail::priority_tag<2>{}))
};

inline constexpr auto p_degree = p_degree_msvc{};

#else

inline constexpr auto p_degree = [](auto &&x, const symbol_set &ss)
    PIRANHA_SS_FORWARD_LAMBDA(detail::p_degree_impl(::std::forward<decltype(x)>(x), ss, detail::priority_tag<2>{}));

#endif

namespace detail
{

template <typename T>
using p_degree_t = decltype(::piranha::p_degree(::std::declval<T>(), ::std::declval<const symbol_set &>()));

}

template <typename T>
using is_with_p_degree = is_detected<detail::p_degree_t, T>;

template <typename T>
inline constexpr bool is_with_p_degree_v = is_with_p_degree<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL WithPDegree = requires(T &&x, const symbol_set &ss)
{
    ::piranha::p_degree(::std::forward<T>(x), ss);
};

#endif

} // namespace piranha

#endif
