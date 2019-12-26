// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_FMA3_HPP
#define OBAKE_MATH_FMA3_HPP

#include <cmath>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>

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

// External customisation point for obake::fma3().
template <typename T, typename U, typename V
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto fma3 = not_implemented;

} // namespace customisation

namespace detail
{

// Implementation for C++ floating-point types.
// Will use the std::fma() family of functions,
// if they have fast implementations.
#if defined(FP_FAST_FMAF)

inline void fma3(float &ret, const float &x, const float &y)
{
    ret = ::std::fmaf(x, y, ret);
}

#endif

#if defined(FP_FAST_FMA)

inline void fma3(double &ret, const double &x, const double &y)
{
    ret = ::std::fma(x, y, ret);
}

#endif

#if defined(FP_FAST_FMAL)

inline void fma3(long double &ret, const long double &x, const long double &y)
{
    ret = ::std::fmal(x, y, ret);
}

#endif

// Implementation for the mp++ classes.
template <::std::size_t SSize>
inline void fma3(::mppp::integer<SSize> &ret, const ::mppp::integer<SSize> &x, const ::mppp::integer<SSize> &y)
{
    ::mppp::addmul(ret, x, y);
}

#if defined(MPPP_WITH_MPFR)

inline void fma3(::mppp::real &ret, const ::mppp::real &x, const ::mppp::real &y)
{
    ::mppp::fma(ret, x, y, ret);
}

#endif

#if defined(MPPP_WITH_QUADMATH)

inline void fma3(::mppp::real128 &ret, const ::mppp::real128 &x, const ::mppp::real128 &y)
{
    ret = ::mppp::fma(x, y, ret);
}

#endif

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U, typename V>
constexpr auto fma3_impl(T &&x, U &&y, V &&z, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::fma3<T &&, U &&, V &&>)(::std::forward<T>(x), ::std::forward<U>(y),
                                                                      ::std::forward<V>(z)));

// Unqualified function call implementation.
template <typename T, typename U, typename V>
constexpr auto fma3_impl(T &&x, U &&y, V &&z, priority_tag<0>)
    OBAKE_SS_FORWARD_FUNCTION(fma3(::std::forward<T>(x), ::std::forward<U>(y), ::std::forward<V>(z)));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct fma3_msvc {
    template <typename T, typename U, typename V>
    constexpr auto operator()(T &&x, U &&y, V &&z) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(void(detail::fma3_impl(::std::forward<T>(x), ::std::forward<U>(y),
                                                                ::std::forward<V>(z), detail::priority_tag<1>{})))
};

inline constexpr auto fma3 = fma3_msvc{};

#else

inline constexpr auto fma3 = [](auto &&x, auto &&y, auto &&z)
    OBAKE_SS_FORWARD_LAMBDA(void(detail::fma3_impl(::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y),
                                                   ::std::forward<decltype(z)>(z), detail::priority_tag<1>{})));

#endif

namespace detail
{

template <typename T, typename U, typename V>
using fma3_t = decltype(::obake::fma3(::std::declval<T>(), ::std::declval<U>(), ::std::declval<V>()));

}

template <typename T, typename U, typename V>
using is_mult_addable = is_detected<detail::fma3_t, T, U, V>;

template <typename T, typename U, typename V>
inline constexpr bool is_mult_addable_v = is_mult_addable<T, U, V>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T, typename U, typename V>
OBAKE_CONCEPT_DECL MultAddable = requires(T &&x, U &&y, V &&z)
{
    ::obake::fma3(::std::forward<T>(x), ::std::forward<U>(y), ::std::forward<V>(z));
};

#endif

} // namespace obake

#endif
