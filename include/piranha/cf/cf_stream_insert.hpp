// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_CF_CF_STREAM_INSERT_HPP
#define PIRANHA_CF_CF_STREAM_INSERT_HPP

#include <ostream>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/not_implemented.hpp>
#include <piranha/detail/priority_tag.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace customisation
{

// External customisation point for piranha::cf_stream_insert().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto cf_stream_insert = not_implemented;

namespace internal
{

// Internal customisation point for piranha::cf_stream_insert().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto cf_stream_insert = not_implemented;

} // namespace internal

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto cf_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<3>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::cf_stream_insert<T &&>)(os, ::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto cf_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<2>)
    PIRANHA_SS_FORWARD_FUNCTION(cf_stream_insert(os, ::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto cf_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<1>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::internal::cf_stream_insert<T &&>)(os, ::std::forward<T>(x)));

// Default implementation.
template <typename T>
constexpr auto cf_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(void(os << ::std::forward<T>(x)));

} // namespace detail

#if defined(_MSC_VER)

struct cf_stream_insert_msvc {
    template <typename T>
    constexpr auto operator()(::std::ostream &os, T &&x) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(void(detail::cf_stream_insert_impl(os, ::std::forward<T>(x),
                                                                              detail::priority_tag<3>{})))
};

inline constexpr auto cf_stream_insert = cf_stream_insert_msvc{};

#else

inline constexpr auto cf_stream_insert = [](::std::ostream & os, auto &&x) PIRANHA_SS_FORWARD_LAMBDA(
    void(detail::cf_stream_insert_impl(os, ::std::forward<decltype(x)>(x), detail::priority_tag<3>{})));

#endif

namespace detail
{

template <typename T>
using cf_stream_insert_t
    = decltype(::piranha::cf_stream_insert(::std::declval<::std::ostream &>(), ::std::declval<T>()));

}

template <typename T>
using is_stream_insertable_cf = is_detected<detail::cf_stream_insert_t, T>;

template <typename T>
inline constexpr bool is_stream_insertable_cf_v = is_stream_insertable_cf<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL StreamInsertableCf = requires(::std::ostream &os, T &&x)
{
    ::piranha::cf_stream_insert(os, ::std::forward<T>(x));
};

#endif

} // namespace piranha

#endif
