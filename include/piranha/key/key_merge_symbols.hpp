// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_KEY_KEY_MERGE_SYMBOLS_HPP
#define PIRANHA_KEY_KEY_MERGE_SYMBOLS_HPP

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

// External customisation point for piranha::key_merge_symbols().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
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
    PIRANHA_SS_FORWARD_FUNCTION((customisation::key_merge_symbols<T &&>)(::std::forward<T>(x), ins_map, ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto key_merge_symbols_impl(T &&x, const symbol_idx_map<symbol_set> &ins_map, const symbol_set &ss,
                                      priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(key_merge_symbols(::std::forward<T>(x), ins_map, ss));

template <typename T>
using key_merge_symbols_impl_ret_t
    = decltype(detail::key_merge_symbols_impl(::std::declval<T>(), ::std::declval<const symbol_idx_map<symbol_set> &>(),
                                              ::std::declval<const symbol_set &>(), priority_tag<1>{}));

template <typename T,
          ::std::enable_if_t<::std::is_same_v<remove_cvref_t<T>, detected_t<key_merge_symbols_impl_ret_t, T>>, int> = 0>
constexpr auto key_merge_symbols_impl_with_ret_check(T &&x, const symbol_idx_map<symbol_set> &ins_map,
                                                     const symbol_set &ss)
    PIRANHA_SS_FORWARD_FUNCTION(detail::key_merge_symbols_impl(::std::forward<T>(x), ins_map, ss, priority_tag<1>{}));

} // namespace detail

#if defined(_MSC_VER)

struct key_merge_symbols_msvc {
    template <typename T>
    constexpr auto operator()(T &&x, const symbol_idx_map<symbol_set> &ins_map, const symbol_set &ss) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(detail::key_merge_symbols_impl_with_ret_check(::std::forward<T>(x), ins_map,
                                                                                         ss))
};

inline constexpr auto key_merge_symbols = key_merge_symbols_msvc{};

#else

inline constexpr auto key_merge_symbols =
    [](auto &&x, const symbol_idx_map<symbol_set> &ins_map, const symbol_set &ss) PIRANHA_SS_FORWARD_LAMBDA(
        detail::key_merge_symbols_impl_with_ret_check(::std::forward<decltype(x)>(x), ins_map, ss));

#endif

namespace detail
{

template <typename T>
using key_merge_symbols_t = decltype(::piranha::key_merge_symbols(
    ::std::declval<T>(), ::std::declval<const symbol_idx_map<symbol_set> &>(), ::std::declval<const symbol_set &>()));

}

// NOTE: runtime requirement: the output key will be
// compatible with the merged symbol set.
template <typename T>
using is_symbols_mergeable_key = is_detected<detail::key_merge_symbols_t, T>;

template <typename T>
inline constexpr bool is_symbols_mergeable_key_v = is_symbols_mergeable_key<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL SymbolsMergeableKey
    = requires(T &&x, const symbol_idx_map<symbol_set> &ins_map, const symbol_set &ss)
{
    ::piranha::key_merge_symbols(::std::forward<T>(x), ins_map, ss);
};

#endif

} // namespace piranha

#endif
