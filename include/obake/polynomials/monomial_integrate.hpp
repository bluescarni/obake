// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POLYNOMIALS_MONOMIAL_INTEGRATE_HPP
#define OBAKE_POLYNOMIALS_MONOMIAL_INTEGRATE_HPP

#include <type_traits>
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

// External customisation point for obake::monomial_integrate().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto monomial_integrate = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto monomial_integrate_impl(T &&x, const symbol_idx &idx, const symbol_set &ss, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::monomial_integrate<T &&>)(::std::forward<T>(x), idx, ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto monomial_integrate_impl(T &&x, const symbol_idx &idx, const symbol_set &ss, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(monomial_integrate(::std::forward<T>(x), idx, ss));

// Machinery to enable the monomial_integrate() implementation only
// if the return value is a std::pair whose second element has
// the same type as the input monomial (after cvref removal).
template <typename T, typename M>
struct is_monomial_integrate_retval : ::std::false_type {
};

template <typename T, typename M>
struct is_monomial_integrate_retval<::std::pair<T, M>, M> : ::std::true_type {
};

template <typename T>
using monomial_integrate_impl_ret_t
    = decltype(detail::monomial_integrate_impl(::std::declval<T>(), ::std::declval<const symbol_idx &>(),
                                               ::std::declval<const symbol_set &>(), priority_tag<1>{}));

template <
    typename T,
    ::std::enable_if_t<
        is_monomial_integrate_retval<detected_t<monomial_integrate_impl_ret_t, T>, remove_cvref_t<T>>::value, int> = 0>
constexpr auto monomial_integrate_impl_with_ret_check(T &&x, const symbol_idx &idx, const symbol_set &ss)
    OBAKE_SS_FORWARD_FUNCTION(detail::monomial_integrate_impl(::std::forward<T>(x), idx, ss, priority_tag<1>{}));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct monomial_integrate_msvc {
    template <typename T>
    constexpr auto operator()(T &&x, const symbol_idx &idx, const symbol_set &ss) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::monomial_integrate_impl_with_ret_check(::std::forward<T>(x), idx, ss))
};

inline constexpr auto monomial_integrate = monomial_integrate_msvc{};

#else

inline constexpr auto monomial_integrate = [](auto &&x, const symbol_idx &idx, const symbol_set &ss)
    OBAKE_SS_FORWARD_LAMBDA(detail::monomial_integrate_impl_with_ret_check(::std::forward<decltype(x)>(x), idx, ss));

#endif

namespace detail
{

template <typename T>
using monomial_integrate_t = decltype(::obake::monomial_integrate(
    ::std::declval<T>(), ::std::declval<const symbol_idx &>(), ::std::declval<const symbol_set &>()));

}

// NOTE: runtime requirement: the returned monomial must be compatible with the reference
// symbol set, and symbol_idx must be smaller than ss.size().
template <typename T>
using is_integrable_monomial = is_detected<detail::monomial_integrate_t, T>;

template <typename T>
inline constexpr bool is_integrable_monomial_v = is_integrable_monomial<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL IntegrableMonomial = requires(T &&x, const symbol_idx &idx, const symbol_set &ss)
{
    ::obake::monomial_integrate(::std::forward<T>(x), idx, ss);
};

#endif

} // namespace obake

#endif
