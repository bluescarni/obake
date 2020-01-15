// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_LIMITS_HPP
#define OBAKE_DETAIL_LIMITS_HPP

#include <limits>

#include <obake/config.hpp>

// Wrappers for some std::numeric_limits properties, extended to support
// 128bit integers.

namespace obake::detail
{

template <typename T>
inline constexpr auto limits_min = ::std::numeric_limits<T>::min();

template <typename T>
inline constexpr auto limits_max = ::std::numeric_limits<T>::max();

#if defined(OBAKE_HAVE_GCC_INT128)

template <>
inline constexpr auto limits_max<__int128_t> = static_cast<__int128_t>((__uint128_t(1) << 127u) - 1u);

template <>
inline constexpr auto limits_min<__int128_t> = -limits_max<__int128_t> - 1;

template <>
inline constexpr auto limits_max<__uint128_t> = ~__uint128_t(0);

template <>
inline constexpr auto limits_min<__uint128_t> = __uint128_t(0);

#endif

template <typename T>
inline constexpr auto limits_digits = ::std::numeric_limits<T>::digits;

#if defined(OBAKE_HAVE_GCC_INT128)

template <>
inline constexpr auto limits_digits<__int128_t> = 127;

template <>
inline constexpr auto limits_digits<__uint128_t> = 128;

#endif

template <typename T>
inline constexpr auto limits_digits10 = ::std::numeric_limits<T>::digits10;

#if defined(OBAKE_HAVE_GCC_INT128)

// NOTE: these can be calculated as digits * log10(2), rounded down:
// https://en.cppreference.com/w/cpp/types/numeric_limits/digits10
template <>
inline constexpr auto limits_digits10<__int128_t> = 38;

template <>
inline constexpr auto limits_digits10<__uint128_t> = 38;

#endif

} // namespace obake::detail

#endif
