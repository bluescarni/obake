// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_MATH_POW_HPP
#define PIRANHA_MATH_POW_HPP

#include <cmath>

namespace piranha
{

namespace cpo_impl
{

template <typename T, typename U, std::enable_if_t<std::is_floating_point_v<T> && std::is_floating_point_v<U>, int> = 0>
inline auto power(T x, U y)
{
    return std::pow(x, y);
}

inline constexpr auto power_f
    = [](auto &&x, auto &&y) -> decltype(power(std::forward<decltype(x)>(x), std::forward<decltype(y)>(y))) {
    return power(std::forward<decltype(x)>(x), std::forward<decltype(y)>(y));
};

} // namespace cpo_impl

} // namespace piranha

#endif
