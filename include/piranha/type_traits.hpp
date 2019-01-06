// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_TYPE_TRAITS_HPP
#define PIRANHA_TYPE_TRAITS_HPP

#include <cstddef>
#include <type_traits>

#include <mp++/detail/type_traits.hpp>

#include <piranha/config.hpp>

namespace piranha
{

// Lift the detection idiom from mp++.
using ::mppp::detected_t;
using ::mppp::is_detected;

// Handy alias.
template <typename T>
using uncvref_t = ::std::remove_cv_t<::std::remove_reference_t<T>>;

template <::std::size_t I>
struct priority_tag : ::piranha::priority_tag<I - 1u> {
};

template <>
struct priority_tag<0> {
};

// Detect C++ integral types, including GCC-style 128bit integers.
template <typename T>
struct is_cpp_integral : ::std::disjunction<::std::is_integral<T>
#if defined(PIRANHA_HAVE_GCC_INT128)
                                            ,
                                            ::std::is_same<::std::remove_cv_t<T>, __int128_t>,
                                            ::std::is_same<::std::remove_cv_t<T>, __uint128_t>
#endif
                                            > {
};

template <typename T>
inline constexpr bool is_cpp_integral_v = ::piranha::is_cpp_integral<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CppIntegral = ::piranha::is_cpp_integral_v<T>;

#endif

// Detect C++ FP types.
template <typename T>
using is_cpp_floating_point = ::std::is_floating_point<T>;

template <typename T>
inline constexpr bool is_cpp_floating_point_v = ::piranha::is_cpp_floating_point<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CppFloatingPoint = ::piranha::is_cpp_floating_point_v<T>;

#endif

// Detect C++ arithmetic types, including GCC-style 128bit integers.
template <typename T>
struct is_cpp_arithmetic : ::std::disjunction<::piranha::is_cpp_integral<T>, ::piranha::is_cpp_floating_point<T>> {
};

template <typename T>
inline constexpr bool is_cpp_arithmetic_v = ::piranha::is_cpp_arithmetic<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CppArithmetic = ::piranha::is_cpp_arithmetic_v<T>;

#endif

} // namespace piranha

#endif