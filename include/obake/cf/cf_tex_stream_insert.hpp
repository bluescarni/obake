// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_CF_CF_TEX_STREAM_INSERT_HPP
#define OBAKE_CF_CF_TEX_STREAM_INSERT_HPP

#include <ostream>
#include <utility>

#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/tex_stream_insert.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::cf_tex_stream_insert().
struct cf_tex_stream_insert_t {
};

namespace internal
{

// Internal customisation point for obake::cf_tex_stream_insert().
struct cf_tex_stream_insert_t {
};

} // namespace internal

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto cf_tex_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<3>)
    OBAKE_SS_FORWARD_FUNCTION(cf_tex_stream_insert(customisation::cf_tex_stream_insert_t{}, os, ::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto cf_tex_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION(cf_tex_stream_insert(os, ::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto cf_tex_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(cf_tex_stream_insert(customisation::internal::cf_tex_stream_insert_t{}, os,
                                                   ::std::forward<T>(x)));

// Default implementation: offload to tex_stream_insert().
template <typename T>
constexpr auto cf_tex_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(::obake::tex_stream_insert(os, ::std::forward<T>(x)));

} // namespace detail

inline constexpr auto cf_tex_stream_insert = []<typename T>(::std::ostream & os, T &&x) OBAKE_SS_FORWARD_LAMBDA(
    void(detail::cf_tex_stream_insert_impl(os, ::std::forward<T>(x), detail::priority_tag<3>{})));

namespace detail
{

template <typename T>
using cf_tex_stream_insert_t
    = decltype(::obake::cf_tex_stream_insert(::std::declval<::std::ostream &>(), ::std::declval<T>()));

}

template <typename T>
using is_tex_stream_insertable_cf = is_detected<detail::cf_tex_stream_insert_t, T>;

template <typename T>
inline constexpr bool is_tex_stream_insertable_cf_v = is_tex_stream_insertable_cf<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
concept tex_stream_insertable_cf = requires(::std::ostream &os, T &&x)
{
    ::obake::cf_tex_stream_insert(os, ::std::forward<T>(x));
};

#endif

} // namespace obake

#endif
