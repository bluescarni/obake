// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POLYNOMIALS_MONOMIAL_RANGE_OVERFLOW_CHECK_HPP
#define OBAKE_POLYNOMIALS_MONOMIAL_RANGE_OVERFLOW_CHECK_HPP

#include <utility>

#include <obake/config.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::monomial_range_overflow_check().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto monomial_range_overflow_check = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto monomial_range_overflow_check_impl(T &&x, U &&y, const symbol_set &ss, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::monomial_range_overflow_check<T &&, U &&>)(::std::forward<T>(x),
                                                                                         ::std::forward<U>(y), ss));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto monomial_range_overflow_check_impl(T &&x, U &&y, const symbol_set &ss, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(monomial_range_overflow_check(::std::forward<T>(x), ::std::forward<U>(y), ss));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct monomial_range_overflow_check_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y, const symbol_set &ss) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(static_cast<bool>(detail::monomial_range_overflow_check_impl(
            ::std::forward<T>(x), ::std::forward<U>(y), ss, detail::priority_tag<1>{})))
};

inline constexpr auto monomial_range_overflow_check = monomial_range_overflow_check_msvc{};

#else

// NOTE: as usual, cast the return value to bool.
inline constexpr auto monomial_range_overflow_check = [](auto &&x, auto &&y, const symbol_set &ss)
    OBAKE_SS_FORWARD_LAMBDA(static_cast<bool>(detail::monomial_range_overflow_check_impl(
        ::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), ss, detail::priority_tag<1>{})));

#endif

namespace detail
{

template <typename T, typename U>
using monomial_range_overflow_check_t = decltype(::obake::monomial_range_overflow_check(
    ::std::declval<T>(), ::std::declval<U>(), ::std::declval<const symbol_set &>()));

}

template <typename T, typename U>
using are_overflow_testable_monomial_ranges = is_detected<detail::monomial_range_overflow_check_t, T, U>;

template <typename T, typename U>
inline constexpr bool are_overflow_testable_monomial_ranges_v = are_overflow_testable_monomial_ranges<T, U>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T, typename U>
OBAKE_CONCEPT_DECL OverflowTestableMonomialRanges = requires(T &&x, U &&y, const symbol_set &ss)
{
    ::obake::monomial_range_overflow_check(::std::forward<T>(x), ::std::forward<U>(y), ss);
};

#endif

} // namespace obake

#endif
