// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_KEY_KEY_IS_ONE_HPP
#define OBAKE_KEY_KEY_IS_ONE_HPP

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

// External customisation point for obake::key_is_one().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto key_is_one = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto key_is_one_impl(T &&x, const symbol_set &ss, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::key_is_one<T &&>)(::std::forward<T>(x), ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto key_is_one_impl(T &&x, const symbol_set &ss, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(key_is_one(::std::forward<T>(x), ss));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct key_is_one_msvc {
    template <typename T>
    constexpr auto operator()(T &&x, const symbol_set &ss) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(static_cast<bool>(detail::key_is_one_impl(::std::forward<T>(x), ss,
                                                                                   detail::priority_tag<1>{})))
};

inline constexpr auto key_is_one = key_is_one_msvc{};

#else

// NOTE: forcibly cast to bool the return value, so that if the selected implementation
// returns a type which is not convertible to bool, this call will SFINAE out.
inline constexpr auto key_is_one = [](auto &&x, const symbol_set &ss) OBAKE_SS_FORWARD_LAMBDA(
    static_cast<bool>(detail::key_is_one_impl(::std::forward<decltype(x)>(x), ss, detail::priority_tag<1>{})));

#endif

namespace detail
{

template <typename T>
using key_is_one_t = decltype(::obake::key_is_one(::std::declval<T>(), ::std::declval<const symbol_set &>()));

}

template <typename T>
using is_one_testable_key = is_detected<detail::key_is_one_t, T>;

template <typename T>
inline constexpr bool is_one_testable_key_v = is_one_testable_key<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL OneTestableKey = requires(T &&x, const symbol_set &ss)
{
    ::obake::key_is_one(::std::forward<T>(x), ss);
};

#endif

} // namespace obake

#endif
