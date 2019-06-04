// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_KEY_KEY_IS_COMPATIBLE_HPP
#define PIRANHA_KEY_KEY_IS_COMPATIBLE_HPP

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

// External customisation point for piranha::key_is_compatible().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto key_is_compatible = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto key_is_compatible_impl(T &&x, const symbol_set &ss, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::key_is_compatible<T &&>)(::std::forward<T>(x), ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto key_is_compatible_impl(T &&x, const symbol_set &ss, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(key_is_compatible(::std::forward<T>(x), ss));

} // namespace detail

#if defined(_MSC_VER)

struct key_is_compatible_msvc {
    template <typename T>
    constexpr auto operator()(T &&x, const symbol_set &ss) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(static_cast<bool>(detail::key_is_compatible_impl(::std::forward<T>(x), ss,
                                                                                            detail::priority_tag<1>{})))
};

inline constexpr auto key_is_compatible = key_is_compatible_msvc{};

#else

// NOTE: forcibly cast to bool the return value, so that if the selected implementation
// returns a type which is not convertible to bool, this call will SFINAE out.
inline constexpr auto key_is_compatible = [](auto &&x, const symbol_set &ss) PIRANHA_SS_FORWARD_LAMBDA(
    static_cast<bool>(detail::key_is_compatible_impl(::std::forward<decltype(x)>(x), ss, detail::priority_tag<1>{})));

#endif

namespace detail
{

template <typename T>
using key_is_compatible_t
    = decltype(::piranha::key_is_compatible(::std::declval<T>(), ::std::declval<const symbol_set &>()));

}

template <typename T>
using is_compatibility_testable_key = is_detected<detail::key_is_compatible_t, T>;

template <typename T>
inline constexpr bool is_compatibility_testable_key_v = is_compatibility_testable_key<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CompatibilityTestableKey = requires(T &&x, const symbol_set &ss)
{
    ::piranha::key_is_compatible(::std::forward<T>(x), ss);
};

#endif

} // namespace piranha

#endif
