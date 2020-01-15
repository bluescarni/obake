// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_EVALUATE_HPP
#define OBAKE_MATH_EVALUATE_HPP

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

// External customisation point for obake::evaluate().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto evaluate = not_implemented;

namespace internal
{

// Internal customisation point for obake::evaluate().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto evaluate = not_implemented;

} // namespace internal

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto evaluate_impl(T &&x, const symbol_map<U> &sm, priority_tag<3>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::evaluate<T &&, U>)(::std::forward<T>(x), sm));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto evaluate_impl(T &&x, const symbol_map<U> &sm, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION(evaluate(::std::forward<T>(x), sm));

// Explicit override in the internal customisation namespace.
template <typename T, typename U>
constexpr auto evaluate_impl(T &&x, const symbol_map<U> &sm, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::internal::evaluate<T &&, U>)(::std::forward<T>(x), sm));

// Lowest-priority: forward-construct the input value.
template <typename T, typename U>
constexpr auto evaluate_impl(T &&x, const symbol_map<U> &, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(remove_cvref_t<T>(::std::forward<T>(x)));

} // namespace detail

// NOTE: we need the functor for this in C++17, because of the
// template parameter U inside symbol_map. In C++20 we could
// use the new lambda template syntax.
struct evaluate_functor {
    // NOTE: place an enabling condition here on U, otherwise
    // if we invoke the type-trait/concept with pathological types
    // for U (e.g., void) then the attempted instantiation of
    // symbol_map<void> in the impl functions above will trigger
    // a hard error. Also, it's easier to just require a semi
    // regular type here for meta-programming purposes.
    template <typename T, typename U, ::std::enable_if_t<is_semi_regular_v<U>, int> = 0>
    constexpr auto operator()(T &&x, const symbol_map<U> &sm) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::evaluate_impl(::std::forward<T>(x), sm, detail::priority_tag<3>{}))
};

inline constexpr auto evaluate = evaluate_functor{};

namespace detail
{

template <typename T, typename U>
using evaluate_t = decltype(::obake::evaluate(::std::declval<T>(), ::std::declval<const symbol_map<U> &>()));

}

template <typename T, typename U>
using is_evaluable = is_detected<detail::evaluate_t, T, U>;

template <typename T, typename U>
inline constexpr bool is_evaluable_v = is_evaluable<T, U>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T, typename U>
OBAKE_CONCEPT_DECL Evaluable = requires(T &&x, const symbol_map<U> &sm)
{
    ::obake::evaluate(::std::forward<T>(x), sm);
};

#endif

} // namespace obake

#endif
