// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_KEY_KEY_EVALUATE_HPP
#define PIRANHA_KEY_KEY_EVALUATE_HPP

#include <type_traits>
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

// External customisation point for piranha::key_evaluate().
template <typename T, typename U
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto key_evaluate = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto key_evaluate_impl(T &&x, const symbol_idx_map<U> &sm, const symbol_set &ss, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::key_evaluate<T &&, U>)(::std::forward<T>(x), sm, ss));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto key_evaluate_impl(T &&x, const symbol_idx_map<U> &sm, const symbol_set &ss, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(key_evaluate(::std::forward<T>(x), sm, ss));

} // namespace detail

// NOTE: we need the functor for this in C++17, because of the
// template parameter U inside symbol_idx_map. In C++20 we could
// use the new lambda template syntax.
struct key_evaluate_functor {
    // NOTE: place an enabling condition here on U, otherwise
    // if we invoke the type-trait/concept with pathological types
    // for U (e.g., void) then the attempted instantiation of
    // symbol_idx_map<void> in the impl functions above will trigger
    // a hard error. Also, it's easier to just require a semi
    // regular type here for meta-programming purposes.
    template <typename T, typename U, ::std::enable_if_t<is_semi_regular_v<U>, int> = 0>
    constexpr auto operator()(T &&x, const symbol_idx_map<U> &sm, const symbol_set &ss) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(detail::key_evaluate_impl(::std::forward<T>(x), sm, ss,
                                                                     detail::priority_tag<1>{}))
};

inline constexpr auto key_evaluate = key_evaluate_functor{};

namespace detail
{

template <typename T, typename U>
using key_evaluate_t = decltype(::piranha::key_evaluate(
    ::std::declval<T>(), ::std::declval<const symbol_idx_map<U> &>(), ::std::declval<const symbol_set &>()));

}

template <typename T, typename U>
using is_evaluable_key = is_detected<detail::key_evaluate_t, T, U>;

template <typename T, typename U>
inline constexpr bool is_evaluable_key_v = is_evaluable_key<T, U>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
PIRANHA_CONCEPT_DECL EvaluableKey = requires(T &&x, const symbol_idx_map<U> &sm, const symbol_set &ss)
{
    ::piranha::key_evaluate(::std::forward<T>(x), sm, ss);
};

#endif

} // namespace piranha

#endif
