// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_TRIM_HPP
#define OBAKE_MATH_TRIM_HPP

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

// External customisation point for obake::trim().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto trim = not_implemented;

namespace internal
{

// Internal customisation point for obake::trim().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto trim = not_implemented;

} // namespace internal

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto trim_impl(T &&x, priority_tag<3>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::trim<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto trim_impl(T &&x, priority_tag<2>) OBAKE_SS_FORWARD_FUNCTION(trim(::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto trim_impl(T &&x, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::internal::trim<T &&>)(::std::forward<T>(x)));

// Lowest-priority: forward-construct the input value.
template <typename T>
constexpr auto trim_impl(T &&x, priority_tag<0>) OBAKE_SS_FORWARD_FUNCTION(remove_cvref_t<T>(::std::forward<T>(x)));

// Machinery to enable the trim() implementation only if the return
// type is the same as the input type (after cvref removal).
template <typename T>
using trim_impl_ret_t = decltype(detail::trim_impl(::std::declval<T>(), priority_tag<3>{}));

template <typename T, ::std::enable_if_t<::std::is_same_v<remove_cvref_t<T>, detected_t<trim_impl_ret_t, T>>, int> = 0>
constexpr auto trim_impl_with_ret_check(T &&x)
    OBAKE_SS_FORWARD_FUNCTION(detail::trim_impl(::std::forward<T>(x), priority_tag<3>{}));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct trim_msvc {
    template <typename T>
    constexpr auto operator()(T &&x) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::trim_impl_with_ret_check(::std::forward<T>(x)))
};

inline constexpr auto trim = trim_msvc{};

#else

inline constexpr auto trim =
    [](auto &&x) OBAKE_SS_FORWARD_LAMBDA(detail::trim_impl_with_ret_check(::std::forward<decltype(x)>(x)));

#endif

namespace detail
{

template <typename T>
using trim_t = decltype(::obake::trim(::std::declval<T>()));

}

template <typename T>
using is_trimmable = is_detected<detail::trim_t, T>;

template <typename T>
inline constexpr bool is_trimmable_v = is_trimmable<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL Trimmable = requires(T &&x)
{
    ::obake::trim(::std::forward<T>(x));
};

#endif

} // namespace obake

#endif
