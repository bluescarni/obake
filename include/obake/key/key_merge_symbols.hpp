// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_KEY_KEY_MERGE_SYMBOLS_HPP
#define OBAKE_KEY_KEY_MERGE_SYMBOLS_HPP

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

// External customisation point for obake::key_merge_symbols().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto key_merge_symbols = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto key_merge_symbols_impl(T &&x, const symbol_idx_map<symbol_set> &ins_map, const symbol_set &ss,
                                      priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::key_merge_symbols<T &&>)(::std::forward<T>(x), ins_map, ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto key_merge_symbols_impl(T &&x, const symbol_idx_map<symbol_set> &ins_map, const symbol_set &ss,
                                      priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(key_merge_symbols(::std::forward<T>(x), ins_map, ss));

template <typename T>
using key_merge_symbols_impl_ret_t
    = decltype(detail::key_merge_symbols_impl(::std::declval<T>(), ::std::declval<const symbol_idx_map<symbol_set> &>(),
                                              ::std::declval<const symbol_set &>(), priority_tag<1>{}));

// NOTE: enable only if the return value is remove_cvref_t<T>.
template <typename T,
          ::std::enable_if_t<::std::is_same_v<remove_cvref_t<T>, detected_t<key_merge_symbols_impl_ret_t, T>>, int> = 0>
constexpr auto key_merge_symbols_impl_with_ret_check(T &&x, const symbol_idx_map<symbol_set> &ins_map,
                                                     const symbol_set &ss)
    OBAKE_SS_FORWARD_FUNCTION(detail::key_merge_symbols_impl(::std::forward<T>(x), ins_map, ss, priority_tag<1>{}));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct key_merge_symbols_msvc {
    template <typename T>
    constexpr auto operator()(T &&x, const symbol_idx_map<symbol_set> &ins_map, const symbol_set &ss) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::key_merge_symbols_impl_with_ret_check(::std::forward<T>(x), ins_map,
                                                                                       ss))
};

inline constexpr auto key_merge_symbols = key_merge_symbols_msvc{};

#else

inline constexpr auto key_merge_symbols = [](auto &&x, const symbol_idx_map<symbol_set> &ins_map, const symbol_set &ss)
    OBAKE_SS_FORWARD_LAMBDA(detail::key_merge_symbols_impl_with_ret_check(::std::forward<decltype(x)>(x), ins_map, ss));

#endif

namespace detail
{

template <typename T>
using key_merge_symbols_t = decltype(::obake::key_merge_symbols(
    ::std::declval<T>(), ::std::declval<const symbol_idx_map<symbol_set> &>(), ::std::declval<const symbol_set &>()));

}

// NOTE: runtime requirements:
// - the output key will be compatible with the merged symbol set,
// - if two keys compared different before the merge, then the merged
//   keys will also compare different (provided the other arguments to
//   key_merge_symbols() are identical),
// - if a key was not zero before the merge, then it will be not
//   zero after the merge.
template <typename T>
using is_symbols_mergeable_key = is_detected<detail::key_merge_symbols_t, T>;

template <typename T>
inline constexpr bool is_symbols_mergeable_key_v = is_symbols_mergeable_key<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL SymbolsMergeableKey
    = requires(T &&x, const symbol_idx_map<symbol_set> &ins_map, const symbol_set &ss)
{
    ::obake::key_merge_symbols(::std::forward<T>(x), ins_map, ss);
};

#endif

} // namespace obake

#endif
