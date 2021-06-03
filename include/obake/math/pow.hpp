// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_POW_HPP
#define OBAKE_MATH_POW_HPP

#include <cmath>
#include <type_traits>
#include <utility>

#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::pow().
struct pow_t {
};

namespace internal
{

// Internal customisation point for obake::pow().
struct pow_t {
};

} // namespace internal

} // namespace customisation

namespace detail
{

// Implementation if one argument is a C++ FP type and the other argument is a C++ arithmetic type.
// NOTE: when obake::pow() is called with a C++ FP+arithmetic combination, then the
// unqualified function call implementation of pow_impl below won't be able to look up pow() in other
// namespaces (as ADL does not kick in for fundamental types), and thus only this specific
// pow() function will be found and used. This is convenient, because it shields us
// from ambiguities with pow() functions defined in the root namespace.
template <typename T, typename U>
requires Arithmetic<T> && Arithmetic<U> &&(FloatingPoint<T> || FloatingPoint<U>)inline auto pow(const T &x,
                                                                                                const U &y) noexcept
{
    // NOTE: when one of the arguments to std::pow() is an integral type,
    // then that argument will be converted to double automatically. This means that,
    // e.g., std::pow(3.f, 4) will give a double result instead of float. This is
    // not consistent with the fact that 3.f + 4 gives a float result, so here
    // we implement a pow() behaviour consistent with the usual promotion rules
    // for mixed integral/fp operations.
    // https://en.cppreference.com/w/cpp/numeric/math/pow
    // NOTE: this will also apply to 128bit integers, if supported.
    if constexpr (is_integral_v<T>) {
        return ::std::pow(static_cast<U>(x), y);
    } else if constexpr (is_integral_v<U>) {
        return ::std::pow(x, static_cast<T>(y));
    } else {
        return ::std::pow(x, y);
    }
}

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto pow_impl(T &&x, U &&y, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION(pow(customisation::pow_t{}, ::std::forward<T>(x), ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto pow_impl(T &&x, U &&y, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(pow(::std::forward<T>(x), ::std::forward<U>(y)));

// Lowest priority: override in the internal customisation namespace.
template <typename T, typename U>
constexpr auto pow_impl(T &&x, U &&y, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(pow(customisation::internal::pow_t{}, ::std::forward<T>(x), ::std::forward<U>(y)));

} // namespace detail

inline constexpr auto pow = [](auto &&x, auto &&y) OBAKE_SS_FORWARD_LAMBDA(
    detail::pow_impl(::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<2>{}));

namespace detail
{

template <typename T, typename U>
using pow_t = decltype(::obake::pow(::std::declval<T>(), ::std::declval<U>()));

}

template <typename T, typename U>
using is_exponentiable = is_detected<detail::pow_t, T, U>;

template <typename T, typename U>
inline constexpr bool is_exponentiable_v = is_exponentiable<T, U>::value;

template <typename T, typename U>
concept Exponentiable = requires(T &&x, U &&y)
{
    ::obake::pow(::std::forward<T>(x), ::std::forward<U>(y));
};

} // namespace obake

#endif
