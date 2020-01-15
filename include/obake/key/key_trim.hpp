// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_KEY_KEY_TRIM_HPP
#define OBAKE_KEY_KEY_TRIM_HPP

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

// External customisation point for obake::key_trim().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto key_trim = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto key_trim_impl(T &&x, const symbol_idx_set &si, const symbol_set &ss, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::key_trim<T &&>)(::std::forward<T>(x), si, ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto key_trim_impl(T &&x, const symbol_idx_set &si, const symbol_set &ss, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(key_trim(::std::forward<T>(x), si, ss));

// Machinery to enable the key_trim implementation only if the return
// type is the same as the input type (after cvref removal).
template <typename T>
using key_trim_impl_ret_t
    = decltype(detail::key_trim_impl(::std::declval<T>(), ::std::declval<const symbol_idx_set &>(),
                                     ::std::declval<const symbol_set &>(), priority_tag<1>{}));

template <typename T,
          ::std::enable_if_t<::std::is_same_v<remove_cvref_t<T>, detected_t<key_trim_impl_ret_t, T>>, int> = 0>
constexpr auto key_trim_impl_with_ret_check(T &&x, const symbol_idx_set &si, const symbol_set &ss)
    OBAKE_SS_FORWARD_FUNCTION(detail::key_trim_impl(::std::forward<T>(x), si, ss, priority_tag<1>{}));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct key_trim_msvc {
    template <typename T>
    constexpr auto operator()(T &&x, const symbol_idx_set &si, const symbol_set &ss) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::key_trim_impl_with_ret_check(::std::forward<T>(x), si, ss))
};

inline constexpr auto key_trim = key_trim_msvc{};

#else

inline constexpr auto key_trim = [](auto &&x, const symbol_idx_set &si, const symbol_set &ss)
    OBAKE_SS_FORWARD_LAMBDA(detail::key_trim_impl_with_ret_check(::std::forward<decltype(x)>(x), si, ss));

#endif

namespace detail
{

template <typename T>
using key_trim_t = decltype(::obake::key_trim(::std::declval<T>(), ::std::declval<const symbol_idx_set &>(),
                                              ::std::declval<const symbol_set &>()));

}

// NOTE: runtime requirement: the result of trimming must be
// compatible with the symbol set resulting from removing
// from ss the symbols at the indices in si.
template <typename T>
using is_trimmable_key = is_detected<detail::key_trim_t, T>;

template <typename T>
inline constexpr bool is_trimmable_key_v = is_trimmable_key<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL TrimmableKey = requires(T &&x, const symbol_idx_set &si, const symbol_set &ss)
{
    ::obake::key_trim(::std::forward<T>(x), si, ss);
};

#endif

} // namespace obake

#endif
