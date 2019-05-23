// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_KEY_KEY_STREAM_INSERT_HPP
#define PIRANHA_KEY_KEY_STREAM_INSERT_HPP

#include <ostream>
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

// External customisation point for piranha::key_stream_insert().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto key_stream_insert = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto key_stream_insert_impl(::std::ostream &os, T &&x, const symbol_set &ss, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::key_stream_insert<T &&>)(os, ::std::forward<T>(x), ss));

// Unqualified function call implementation.
template <typename T>
constexpr auto key_stream_insert_impl(::std::ostream &os, T &&x, const symbol_set &ss, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(key_stream_insert(os, ::std::forward<T>(x), ss));

} // namespace detail

#if defined(_MSC_VER) && !defined(__clang__)

struct key_stream_insert_msvc {
    template <typename T>
    constexpr auto operator()(::std::ostream &os, T &&x, const symbol_set &ss) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(detail::key_stream_insert_impl(os, ::std::forward<T>(x), ss,
                                                                          detail::priority_tag<1>{}))
};

inline constexpr auto key_stream_insert = key_stream_insert_msvc{};

#else

inline constexpr auto key_stream_insert =
    [](::std::ostream & os, auto &&x, const symbol_set &ss) PIRANHA_SS_FORWARD_LAMBDA(
        detail::key_stream_insert_impl(os, ::std::forward<decltype(x)>(x), ss, detail::priority_tag<1>{}));

#endif

namespace detail
{

template <typename T>
using key_stream_insert_t = decltype(::piranha::key_stream_insert(
    ::std::declval<::std::ostream &>(), ::std::declval<T>(), ::std::declval<const symbol_set &>()));

}

template <typename T>
using is_stream_insertable_key = is_detected<detail::key_stream_insert_t, T>;

template <typename T>
inline constexpr bool is_stream_insertable_key_v = is_stream_insertable_key<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL StreamInsertableKey = requires(::std::ostream &os, T &&x, const symbol_set &ss)
{
    ::piranha::key_stream_insert(os, ::std::forward<T>(x), ss);
};

#endif

} // namespace piranha

#endif
