// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_IS_ZERO_HPP
#define OBAKE_MATH_IS_ZERO_HPP

#include <utility>

#include <mp++/config.hpp>

#if defined(MPPP_WITH_MPFR)

// NOTE: mppp::real does not have an is_zero() overload, thus
// we must provide here an implementation.
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

// External customisation point for obake::is_zero().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto is_zero = not_implemented;

namespace internal
{

// Internal customisation point for obake::is_zero().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto is_zero = not_implemented;

} // namespace internal

} // namespace customisation

namespace detail
{

#if defined(MPPP_WITH_MPFR)

// Implementation of is_zero() for mppp::real. It will be found
// by the unqualified function call overload of is_zero_impl().
inline bool is_zero(const ::mppp::real &r)
{
    return r.zero_p();
}

#endif

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto is_zero_impl(T &&x, priority_tag<3>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::is_zero<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto is_zero_impl(T &&x, priority_tag<2>) OBAKE_SS_FORWARD_FUNCTION(is_zero(::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto is_zero_impl(T &&x, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::internal::is_zero<T &&>)(::std::forward<T>(x)));

// Lowest-priority: implementation based on the comparison operator and construction from the zero
// integral constant.
// NOTE: this must go into lowest priority, we want the ADL-based implementation to have
// the precedence.
template <typename T>
constexpr auto is_zero_impl(T &&x, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(::std::forward<T>(x) == remove_cvref_t<T>(0));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct is_zero_msvc {
    template <typename T>
    constexpr auto operator()(T &&x) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(static_cast<bool>(detail::is_zero_impl(::std::forward<T>(x),
                                                                                detail::priority_tag<3>{})))
};

inline constexpr auto is_zero = is_zero_msvc{};

#else

// NOTE: forcibly cast to bool the return value, so that if the selected implementation
// returns a type which is not convertible to bool, this call will SFINAE out.
inline constexpr auto is_zero = [](auto &&x) OBAKE_SS_FORWARD_LAMBDA(
    static_cast<bool>(detail::is_zero_impl(::std::forward<decltype(x)>(x), detail::priority_tag<3>{})));

#endif

namespace detail
{

template <typename T>
using is_zero_t = decltype(::obake::is_zero(::std::declval<T>()));

}

template <typename T>
using is_zero_testable = is_detected<detail::is_zero_t, T>;

template <typename T>
inline constexpr bool is_zero_testable_v = is_zero_testable<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL ZeroTestable = requires(T &&x)
{
    ::obake::is_zero(::std::forward<T>(x));
};

#endif

} // namespace obake

#endif
