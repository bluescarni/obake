// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_INTEGRATE_HPP
#define OBAKE_MATH_INTEGRATE_HPP

#include <string>
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

// External customisation point for obake::integrate().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto integrate = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto integrate_impl(T &&x, const ::std::string &s, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::integrate<T &&>)(::std::forward<T>(x), s));

// Unqualified function call implementation.
template <typename T>
constexpr auto integrate_impl(T &&x, const ::std::string &s, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(integrate(::std::forward<T>(x), s));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct integrate_msvc {
    template <typename T>
    constexpr auto operator()(T &&x, const ::std::string &s) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::integrate_impl(::std::forward<T>(x), s, detail::priority_tag<1>{}))
};

inline constexpr auto integrate = integrate_msvc{};

#else

inline constexpr auto integrate = [](auto &&x, const ::std::string &s)
    OBAKE_SS_FORWARD_LAMBDA(detail::integrate_impl(::std::forward<decltype(x)>(x), s, detail::priority_tag<1>{}));

#endif

namespace detail
{

template <typename T>
using integrate_t = decltype(::obake::integrate(::std::declval<T>(), ::std::declval<const ::std::string &>()));

}

template <typename T>
using is_integrable = is_detected<detail::integrate_t, T>;

template <typename T>
inline constexpr bool is_integrable_v = is_integrable<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL Integrable = requires(T &&x, const ::std::string &s)
{
    ::obake::integrate(::std::forward<T>(x), s);
};

#endif

} // namespace obake

#endif
