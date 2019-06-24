// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_MONOMIAL_MUL_HPP
#define PIRANHA_POLYNOMIALS_MONOMIAL_MUL_HPP

#include <utility>

#include <mp++/config.hpp>

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

// External customisation point for piranha::monomial_mul().
template <typename T, typename U, typename V
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto monomial_mul = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U, typename V>
constexpr auto monomial_mul_impl(T &&x, U &&y, V &&z, const symbol_set &ss, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::monomial_mul<T &&, U &&, V &&>)(::std::forward<T>(x),
                                                                                ::std::forward<U>(y),
                                                                                ::std::forward<V>(z), ss));

// Unqualified function call implementation.
template <typename T, typename U, typename V>
constexpr auto monomial_mul_impl(T &&x, U &&y, V &&z, const symbol_set &ss, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(monomial_mul(::std::forward<T>(x), ::std::forward<U>(y), ::std::forward<V>(z), ss));

} // namespace detail

#if defined(_MSC_VER)

struct monomial_mul_msvc {
    template <typename T, typename U, typename V>
    constexpr auto operator()(T &&x, U &&y, V &&z, const symbol_set &ss) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(void(detail::monomial_mul_impl(::std::forward<T>(x), ::std::forward<U>(y),
                                                                          ::std::forward<V>(z), ss,
                                                                          detail::priority_tag<1>{})))
};

inline constexpr auto monomial_mul = monomial_mul_msvc{};

#else

// NOTE: as usual, cast the return value to void in order to ensure
// it is never used.
inline constexpr auto monomial_mul = [](auto &&x, auto &&y, auto &&z, const symbol_set &ss) PIRANHA_SS_FORWARD_LAMBDA(
    void(detail::monomial_mul_impl(::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y),
                                   ::std::forward<decltype(z)>(z), ss, detail::priority_tag<1>{})));

#endif

namespace detail
{

template <typename T, typename U, typename V>
using monomial_mul_t = decltype(::piranha::monomial_mul(::std::declval<T>(), ::std::declval<U>(), ::std::declval<V>(),
                                                        ::std::declval<const symbol_set &>()));

}

template <typename T, typename U, typename V>
using is_multipliable_monomial = is_detected<detail::monomial_mul_t, T, U, V>;

template <typename T, typename U, typename V>
inline constexpr bool is_multipliable_monomial_v = is_multipliable_monomial<T, U, V>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U, typename V>
PIRANHA_CONCEPT_DECL MultipliableMonomial = requires(T &&x, U &&y, V &&z, const symbol_set &ss)
{
    ::piranha::monomial_mul(::std::forward<T>(x), ::std::forward<U>(y), ::std::forward<V>(z), ss);
};

#endif

} // namespace piranha

#endif
