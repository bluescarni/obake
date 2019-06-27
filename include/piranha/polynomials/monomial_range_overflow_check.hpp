// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_MONOMIAL_RANGE_OVERFLOW_CHECK_HPP
#define PIRANHA_POLYNOMIALS_MONOMIAL_RANGE_OVERFLOW_CHECK_HPP

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

// External customisation point for piranha::monomial_range_overflow_check().
template <typename T, typename U
#if !defined(PIRANHA_HAVE_CONCEPTS)
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
constexpr auto monomial_range_overflow_check_impl(T &&x, U &&y, const symbol_set &ss, priority_tag<2>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::monomial_range_overflow_check<T &&, U &&>)(::std::forward<T>(x),
                                                                                           ::std::forward<U>(y), ss));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto monomial_range_overflow_check_impl(T &&x, U &&y, const symbol_set &ss, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION(monomial_range_overflow_check(::std::forward<T>(x), ::std::forward<U>(y), ss));

// Lowest priority: default implementation, returns true.
template <typename T, typename U>
constexpr bool monomial_range_overflow_check_impl(T &&, U &&, const symbol_set &, priority_tag<0>) noexcept
{
    return true;
}

} // namespace detail

#if defined(_MSC_VER)

struct monomial_range_overflow_check_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y, const symbol_set &ss) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(static_cast<bool>(detail::monomial_range_overflow_check_impl(
            ::std::forward<T>(x), ::std::forward<U>(y), ss, detail::priority_tag<2>{})))
};

inline constexpr auto monomial_range_overflow_check = monomial_range_overflow_check_msvc{};

#else

// NOTE: as usual, cast the return value to bool.
inline constexpr auto monomial_range_overflow_check = [](auto &&x, auto &&y, const symbol_set &ss)
    PIRANHA_SS_FORWARD_LAMBDA(static_cast<bool>(detail::monomial_range_overflow_check_impl(
        ::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), ss, detail::priority_tag<2>{})));

#endif

namespace detail
{

template <typename T, typename U>
using monomial_range_overflow_check_t = decltype(::piranha::monomial_range_overflow_check(
    ::std::declval<T>(), ::std::declval<U>(), ::std::declval<const symbol_set &>()));

}

template <typename T, typename U>
using is_overflow_testable_monomial_range = is_detected<detail::monomial_range_overflow_check_t, T, U>;

template <typename T, typename U>
inline constexpr bool is_overflow_testable_monomial_range_v = is_overflow_testable_monomial_range<T, U>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
PIRANHA_CONCEPT_DECL OverflowTestableMonomialRange = requires(T &&x, U &&y, const symbol_set &ss)
{
    ::piranha::monomial_range_overflow_check(::std::forward<T>(x), ::std::forward<U>(y), ss);
};

#endif

} // namespace piranha

#endif
