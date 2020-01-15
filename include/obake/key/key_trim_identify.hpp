// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_KEY_KEY_TRIM_IDENTIFY_HPP
#define OBAKE_KEY_KEY_TRIM_IDENTIFY_HPP

#include <utility>
#include <vector>

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

// External customisation point for obake::key_trim_identify().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto key_trim_identify = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto key_trim_identify_impl(::std::vector<int> &v, T &&x, const symbol_set &ss, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::key_trim_identify<T &&>)(v, ::std::forward<T>(x), ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto key_trim_identify_impl(::std::vector<int> &v, T &&x, const symbol_set &ss, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(key_trim_identify(v, ::std::forward<T>(x), ss));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct key_trim_identify_msvc {
    template <typename T>
    constexpr auto operator()(::std::vector<int> &v, T &&x, const symbol_set &ss) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(void(detail::key_trim_identify_impl(v, ::std::forward<T>(x), ss,
                                                                             detail::priority_tag<1>{})))
};

inline constexpr auto key_trim_identify = key_trim_identify_msvc{};

#else

// NOTE: forcibly cast to void the return value, in order to ensure any return value
// will be ignored.
inline constexpr auto key_trim_identify =
    [](::std::vector<int> & v, auto &&x, const symbol_set &ss) OBAKE_SS_FORWARD_LAMBDA(
        void(detail::key_trim_identify_impl(v, ::std::forward<decltype(x)>(x), ss, detail::priority_tag<1>{})));

#endif

namespace detail
{

template <typename T>
using key_trim_identify_t = decltype(::obake::key_trim_identify(
    ::std::declval<::std::vector<int> &>(), ::std::declval<T>(), ::std::declval<const symbol_set &>()));

}

template <typename T>
using is_trim_identifiable_key = is_detected<detail::key_trim_identify_t, T>;

template <typename T>
inline constexpr bool is_trim_identifiable_key_v = is_trim_identifiable_key<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL TrimIdentifiableKey = requires(::std::vector<int> &v, T &&x, const symbol_set &ss)
{
    ::obake::key_trim_identify(v, ::std::forward<T>(x), ss);
};

#endif

} // namespace obake

#endif
