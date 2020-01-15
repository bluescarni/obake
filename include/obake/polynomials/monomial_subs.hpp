// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POLYNOMIALS_MONOMIAL_SUBS_HPP
#define OBAKE_POLYNOMIALS_MONOMIAL_SUBS_HPP

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

// External customisation point for obake::monomial_subs().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto monomial_subs = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto monomial_subs_impl(T &&x, const symbol_idx_map<U> &sm, const symbol_set &ss, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::monomial_subs<T &&, U>)(::std::forward<T>(x), sm, ss));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto monomial_subs_impl(T &&x, const symbol_idx_map<U> &sm, const symbol_set &ss, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(monomial_subs(::std::forward<T>(x), sm, ss));

// Determine if the type T is a valid
// return type for a monomial substitution
// involving the monomial type M
template <typename T, typename M>
struct is_monomial_subs_retval : ::std::false_type {
};

template <typename T, typename M>
struct is_monomial_subs_retval<::std::pair<T, M>, M> : ::std::true_type {
};

// Machinery to enable the monomial_subs() implementation only
// if the return value is a std::pair whose second element has
// the same type as the input monomial (after cvref removal).
template <typename T, typename U>
using monomial_subs_impl_ret_t
    = decltype(detail::monomial_subs_impl(::std::declval<T>(), ::std::declval<const symbol_idx_map<U> &>(),
                                          ::std::declval<const symbol_set &>(), priority_tag<1>{}));

template <typename T, typename U,
          ::std::enable_if_t<
              is_monomial_subs_retval<detected_t<monomial_subs_impl_ret_t, T, U>, remove_cvref_t<T>>::value, int> = 0>
constexpr auto monomial_subs_impl_with_ret_check(T &&x, const symbol_idx_map<U> &sm, const symbol_set &ss)
    OBAKE_SS_FORWARD_FUNCTION(detail::monomial_subs_impl(::std::forward<T>(x), sm, ss, priority_tag<1>{}));

} // namespace detail

// NOTE: we need the functor for this in C++17, because of the
// template parameter U inside symbol_idx_map. In C++20 we could
// use the new lambda template syntax.
struct monomial_subs_functor {
    // NOTE: place an enabling condition here on U, otherwise
    // if we invoke the type-trait/concept with pathological types
    // for U (e.g., void) then the attempted instantiation of
    // symbol_idx_map<void> in the impl functions above will trigger
    // a hard error. Also, it's easier to just require a semi
    // regular type here for meta-programming purposes.
    template <typename T, typename U, ::std::enable_if_t<is_semi_regular_v<U>, int> = 0>
    constexpr auto operator()(T &&x, const symbol_idx_map<U> &sm, const symbol_set &ss) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::monomial_subs_impl_with_ret_check(::std::forward<T>(x), sm, ss))
};

inline constexpr auto monomial_subs = monomial_subs_functor{};

namespace detail
{

template <typename T, typename U>
using monomial_subs_t = decltype(::obake::monomial_subs(
    ::std::declval<T>(), ::std::declval<const symbol_idx_map<U> &>(), ::std::declval<const symbol_set &>()));

}

// NOTE: runtime requirement: the returned monomial must be compatible with the reference
// symbol set.
template <typename T, typename U>
using is_substitutable_monomial = is_detected<detail::monomial_subs_t, T, U>;

template <typename T, typename U>
inline constexpr bool is_substitutable_monomial_v = is_substitutable_monomial<T, U>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T, typename U>
OBAKE_CONCEPT_DECL SubstitutableMonomial = requires(T &&x, const symbol_idx_map<U> &sm, const symbol_set &ss)
{
    ::obake::monomial_subs(::std::forward<T>(x), sm, ss);
};

#endif

} // namespace obake

#endif
