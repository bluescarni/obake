// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_NEGATE_HPP
#define OBAKE_MATH_NEGATE_HPP

#include <cstddef>
#include <type_traits>
#include <utility>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#if defined(MPPP_WITH_MPFR)
#include <mp++/real.hpp>
#endif

#include <obake/config.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::negate().
struct negate_t {
};

namespace internal
{

// Internal customisation point for obake::negate().
struct negate_t {
};

} // namespace internal

} // namespace customisation

namespace detail
{

// negate() implementations for the mp++ types.
// For real128, the default implementation is ok.
//
// NOTE: we need overloads for both lvalue
// and rvalue references, otherwise
// the unqualified implementation below is skipped
// when an rvalue is passed from the top level function
// (cannot bind an rvalue reference to an lvalue reference).
template <::std::size_t SSize>
inline void negate(::mppp::integer<SSize> &n)
{
    n.neg();
}

template <::std::size_t SSize>
inline void negate(::mppp::integer<SSize> &&n)
{
    n.neg();
}

template <::std::size_t SSize>
inline void negate(::mppp::rational<SSize> &q)
{
    q.neg();
}

template <::std::size_t SSize>
inline void negate(::mppp::rational<SSize> &&q)
{
    q.neg();
}

#if defined(MPPP_WITH_MPFR)

// NOTE: use templates because otherwise implict conversions
// in mppp::real create issues.
template <typename T>
requires(::std::is_same_v<T, ::mppp::real>) inline void negate(T &r)
{
    r.neg();
}

template <typename T>
requires(::std::is_same_v<T &&, ::mppp::real &&>) inline void negate(T &&r)
{
    r.neg();
}

#endif

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto negate_impl(T &&x, priority_tag<3>)
    OBAKE_SS_FORWARD_FUNCTION(negate(customisation::negate_t{}, ::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto negate_impl(T &&x, priority_tag<2>) OBAKE_SS_FORWARD_FUNCTION(negate(::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto negate_impl(T &&x, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(negate(customisation::internal::negate_t{}, ::std::forward<T>(x)));

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(push)
#pragma warning(disable : 4146)

#endif

// Lowest-priority: implementation based on unary minus + assignment.
// NOTE: this must go into lowest priority, we want the ADL-based implementation to have
// the precedence.
template <typename T>
constexpr auto negate_impl(T &&x, priority_tag<0>) OBAKE_SS_FORWARD_FUNCTION(x = -::std::forward<T>(x));

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(pop)

#endif

} // namespace detail

// NOTE: we return a perfectly forwarded reference to x, that is, the
// return type is decltype(x) &&.
inline constexpr auto negate = [](auto &&x)
    OBAKE_SS_FORWARD_LAMBDA((void(detail::negate_impl(::std::forward<decltype(x)>(x), detail::priority_tag<3>{})),
                             ::std::forward<decltype(x)>(x)));

namespace detail
{

template <typename T>
using negate_t = decltype(::obake::negate(::std::declval<T>()));

}

// NOTE: runtime requirement: negation of a non-zero entity
// never results in a zero.
template <typename T>
using is_negatable = is_detected<detail::negate_t, T>;

template <typename T>
inline constexpr bool is_negatable_v = is_negatable<T>::value;

template <typename T>
concept Negatable = requires(T &&x)
{
    ::obake::negate(::std::forward<T>(x));
};

} // namespace obake

#endif
