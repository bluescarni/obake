// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
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
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto negate = not_implemented;

namespace internal
{

// Internal customisation point for obake::negate().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto negate = not_implemented;

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

inline void negate(::mppp::real &r)
{
    r.neg();
}

inline void negate(::mppp::real &&r)
{
    r.neg();
}

#endif

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto negate_impl(T &&x, priority_tag<3>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::negate<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto negate_impl(T &&x, priority_tag<2>) OBAKE_SS_FORWARD_FUNCTION(negate(::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto negate_impl(T &&x, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::internal::negate<T &&>)(::std::forward<T>(x)));

// Lowest-priority: implementation based on unary minus + assignment.
// NOTE: this must go into lowest priority, we want the ADL-based implementation to have
// the precedence.
template <typename T>
constexpr auto negate_impl(T &&x, priority_tag<0>) OBAKE_SS_FORWARD_FUNCTION(x = -::std::forward<T>(x));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

namespace detail
{

template <typename T>
using negate_impl_t = decltype(detail::negate_impl(::std::declval<T>(), priority_tag<3>{}));

}

struct negate_msvc {
    template <typename T, ::std::enable_if_t<is_detected_v<detail::negate_impl_t, T>, int> = 0>
    constexpr T &&operator()(T &&x) const
        noexcept(noexcept(detail::negate_impl(::std::forward<T>(x), detail::priority_tag<3>{})))
    {
        detail::negate_impl(::std::forward<T>(x), detail::priority_tag<3>{});
        return ::std::forward<T>(x);
    }
};

inline constexpr auto negate = negate_msvc{};

#else

// NOTE: we return a perfectly forwarded reference to x, that is, the
// return type is decltype(x) &&.
inline constexpr auto negate = [](auto &&x)
    OBAKE_SS_FORWARD_LAMBDA((void(detail::negate_impl(::std::forward<decltype(x)>(x), detail::priority_tag<3>{})),
                             ::std::forward<decltype(x)>(x)));

#endif

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

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL Negatable = requires(T &&x)
{
    ::obake::negate(::std::forward<T>(x));
};

#endif

} // namespace obake

#endif
