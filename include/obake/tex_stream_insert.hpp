// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_TEX_STREAM_INSERT_HPP
#define OBAKE_TEX_STREAM_INSERT_HPP

#include <cassert>
#include <cstddef>
#include <ostream>
#include <utility>

#include <mp++/rational.hpp>

#include <obake/config.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/detail/visibility.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::tex_stream_insert().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto tex_stream_insert = not_implemented;

namespace internal
{

// Internal customisation point for obake::tex_stream_insert().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto tex_stream_insert = not_implemented;

} // namespace internal

} // namespace customisation

namespace detail
{

#if defined(OBAKE_HAVE_GCC_INT128)

// Implementation for 128-bit integers.
OBAKE_DLL_PUBLIC void tex_stream_insert(::std::ostream &, const __int128_t &);
OBAKE_DLL_PUBLIC void tex_stream_insert(::std::ostream &, const __uint128_t &);

#endif

// Implementation for mppp::rational.
template <::std::size_t SSize>
inline void tex_stream_insert(::std::ostream &os, const ::mppp::rational<SSize> &q)
{
    if (q.get_den().is_one()) {
        os << q.get_num();
    } else {
        const auto sgn = q.get_num().sgn();
        // NOTE: the sign must be strictly positive/negative,
        // because if q is zero then its den is 1 and we
        // end up in the above branch.
        assert(sgn == 1 || sgn == -1);

        if (sgn == 1) {
            os << "\\frac{" << q.get_num();
        } else {
            os << "-\\frac{" << -q.get_num();
        }
        os << "}{" << q.get_den() << '}';
    }
}

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto tex_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<3>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::tex_stream_insert<T &&>)(os, ::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto tex_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION(tex_stream_insert(os, ::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto tex_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::internal::tex_stream_insert<T &&>)(os, ::std::forward<T>(x)));

// Default implementation: just do a plain stream insertion.
template <typename T>
constexpr auto tex_stream_insert_impl(::std::ostream &os, T &&x, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(void(os << ::std::forward<T>(x)));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct tex_stream_insert_msvc {
    template <typename T>
    constexpr auto operator()(::std::ostream &os, T &&x) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(void(detail::tex_stream_insert_impl(os, ::std::forward<T>(x),
                                                                             detail::priority_tag<3>{})))
};

inline constexpr auto tex_stream_insert = tex_stream_insert_msvc{};

#else

// NOTE: perhaps here, and in the other stream insertion functions,
// it would be more ergonomic to return a reference to os. Let's keep
// it in mind for the future.
inline constexpr auto tex_stream_insert = [](::std::ostream & os, auto &&x) OBAKE_SS_FORWARD_LAMBDA(
    void(detail::tex_stream_insert_impl(os, ::std::forward<decltype(x)>(x), detail::priority_tag<3>{})));

#endif

namespace detail
{

template <typename T>
using tex_stream_insert_t
    = decltype(::obake::tex_stream_insert(::std::declval<::std::ostream &>(), ::std::declval<T>()));

}

template <typename T>
using is_tex_stream_insertable = is_detected<detail::tex_stream_insert_t, T>;

template <typename T>
inline constexpr bool is_tex_stream_insertable_v = is_tex_stream_insertable<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL TexStreamInsertable = requires(::std::ostream &os, T &&x)
{
    ::obake::tex_stream_insert(os, ::std::forward<T>(x));
};

#endif

} // namespace obake

#endif
