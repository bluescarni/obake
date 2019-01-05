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
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/cp_func_forward.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace cp
{

// Implementation if one argument is a C++ FP type, and the other argument is a C++ arithmetic type.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T, typename U>
requires ::piranha::CppArithmetic<T> && ::piranha::CppArithmetic<U> && (::piranha::CppFloatingPoint<T> || ::piranha::CppFloatingPoint<U>)
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<::piranha::is_cpp_arithmetic<T>, ::piranha::is_cpp_arithmetic<U>,
                                                  ::std::disjunction<::piranha::is_cpp_floating_point<T>,
                                                                     ::piranha::is_cpp_floating_point<U>>>,
                             int> = 0>
#endif
inline auto pow(const T &x, const U &y) noexcept
{
#if defined(PIRANHA_HAVE_GCC_INT128)
    // NOTE: for consistency with the behaviour of std::pow(), we cast
    // 128bit ints to double before feeding them to std::pow(). This also
    // means that any pow() call involving 128bit ints will have a result
    // of type at least double.
    // https://en.cppreference.com/w/cpp/numeric/math/pow
    if constexpr (::std::disjunction_v<::std::is_same<T, __int128_t>, ::std::is_same<T, __uint128_t>>) {
        return ::std::pow(static_cast<double>(x), y);
    } else if constexpr (::std::disjunction_v<::std::is_same<U, __int128_t>, ::std::is_same<U, __uint128_t>>) {
        return ::std::pow(x, static_cast<double>(y));
    } else {
#endif
        return ::std::pow(x, y);
#if defined(PIRANHA_HAVE_GCC_INT128)
    }
#endif
}

inline constexpr auto pow_func =
    [](auto &&x, auto &&y) PIRANHA_CP_FUNC_FORWARD(pow(::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y)));

} // namespace cp

inline constexpr auto &pow = ::piranha::cp::pow_func;

namespace detect
{

template <typename T, typename U>
using pow_t = decltype(::piranha::pow(::std::declval<T>(), ::std::declval<U>()));

}

template <typename T, typename U>
using is_exponentiable = ::piranha::is_detected<::piranha::detect::pow_t, T, U>;

template <typename T, typename U>
inline constexpr bool is_exponentiable_v = ::piranha::is_exponentiable<T, U>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
PIRANHA_CONCEPT_DECL Exponentiable = ::piranha::is_exponentiable_v<T, U>;

#endif

} // namespace piranha

#endif
