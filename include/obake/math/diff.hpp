// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_DIFF_HPP
#define OBAKE_MATH_DIFF_HPP

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#if defined(MPPP_WITH_MPFR)

#include <mp++/real.hpp>

#endif

#if defined(MPPP_WITH_QUADMATH)

#include <mp++/real128.hpp>

#endif

#include <obake/config.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::diff().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto diff = not_implemented;

} // namespace customisation

namespace detail
{

// Implementation for C++ arithmetic types (including 128-bit integers).
template <typename T, ::std::enable_if_t<is_arithmetic_v<T>, int> = 0>
constexpr T diff(const T &, const ::std::string &)
{
    return T(0);
}

// Implementation for mp++ classes.
template <::std::size_t SSize>
inline ::mppp::integer<SSize> diff(const ::mppp::integer<SSize> &, const ::std::string &)
{
    return ::mppp::integer<SSize>{};
}

template <::std::size_t SSize>
inline ::mppp::rational<SSize> diff(const ::mppp::rational<SSize> &, const ::std::string &)
{
    return ::mppp::rational<SSize>{};
}

#if defined(MPPP_WITH_MPFR)

inline ::mppp::real diff(const ::mppp::real &x, const ::std::string &)
{
    // NOTE: return a zero with the same precision as x.
    return ::mppp::real(0, x.get_prec());
}

#endif

#if defined(MPPP_WITH_QUADMATH)

constexpr ::mppp::real128 diff(const ::mppp::real128 &, const ::std::string &)
{
    return ::mppp::real128(0);
}

#endif

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto diff_impl(T &&x, const ::std::string &s, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::diff<T &&>)(::std::forward<T>(x), s));

// Unqualified function call implementation.
template <typename T>
constexpr auto diff_impl(T &&x, const ::std::string &s, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(diff(::std::forward<T>(x), s));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct diff_msvc {
    template <typename T>
    constexpr auto operator()(T &&x, const ::std::string &s) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::diff_impl(::std::forward<T>(x), s, detail::priority_tag<1>{}))
};

inline constexpr auto diff = diff_msvc{};

#else

inline constexpr auto diff = [](auto &&x, const ::std::string &s)
    OBAKE_SS_FORWARD_LAMBDA(detail::diff_impl(::std::forward<decltype(x)>(x), s, detail::priority_tag<1>{}));

#endif

namespace detail
{

template <typename T>
using diff_t = decltype(::obake::diff(::std::declval<T>(), ::std::declval<const ::std::string &>()));

}

template <typename T>
using is_differentiable = is_detected<detail::diff_t, T>;

template <typename T>
inline constexpr bool is_differentiable_v = is_differentiable<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL Differentiable = requires(T &&x, const ::std::string &s)
{
    ::obake::diff(::std::forward<T>(x), s);
};

#endif

} // namespace obake

#endif
