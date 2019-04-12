// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_LIMITS_HPP
#define PIRANHA_DETAIL_LIMITS_HPP

#include <limits>
#include <tuple>

#include <piranha/config.hpp>

// Wrappers for some std::numeric_limits properties, extended to support
/// 128bit integers.

namespace piranha::detail
{

template <typename T>
inline constexpr auto limits_minmax = ::std::tuple{::std::numeric_limits<T>::min(), ::std::numeric_limits<T>::max()};

#if defined(PIRANHA_HAVE_GCC_INT128)

inline constexpr auto max_int128_t = static_cast<__int128_t>((__uint128_t(1) << 127u) - 1u);

template <>
inline constexpr auto limits_minmax<__int128_t> = ::std::tuple{-max_int128_t - 1, max_int128_t};

template <>
inline constexpr auto limits_minmax<__uint128_t> = ::std::tuple{__uint128_t(0), ~__uint128_t(0)};

#endif

template <typename T>
inline constexpr auto limits_digits = ::std::numeric_limits<T>::digits;

#if defined(PIRANHA_HAVE_GCC_INT128)

template <>
inline constexpr auto limits_digits<__int128_t> = 127;

template <>
inline constexpr auto limits_digits<__uint128_t> = 128;

#endif

} // namespace piranha::detail

#endif
