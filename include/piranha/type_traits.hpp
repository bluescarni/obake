// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_TYPE_TRAITS_HPP
#define PIRANHA_TYPE_TRAITS_HPP

#include <limits>
#include <tuple>
#include <type_traits>

#include <mp++/detail/type_traits.hpp>

#include <piranha/config.hpp>

namespace piranha
{

// Lift the detection idiom from mp++.
using ::mppp::detected_t;
using ::mppp::is_detected;

template <template <class...> class Op, class... Args>
inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

// Handy alias.
template <typename T>
using remove_cvref_t = ::std::remove_cv_t<::std::remove_reference_t<T>>;

// Detect if T and U, after the removal of reference and cv qualifiers, are the same type.
template <typename T, typename U>
using is_same_cvref = ::std::is_same<remove_cvref_t<T>, remove_cvref_t<U>>;

template <typename T, typename U>
inline constexpr bool is_same_cvref_v = is_same_cvref<T, U>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
PIRANHA_CONCEPT_DECL SameCvref = is_same_cvref_v<T, U>;

#endif

// Detect C++ integral types, including GCC-style 128bit integers.
template <typename T>
using is_cpp_integral = ::std::disjunction<::std::is_integral<T>
#if defined(PIRANHA_HAVE_GCC_INT128)
                                           ,
                                           ::std::is_same<::std::remove_cv_t<T>, __int128_t>,
                                           ::std::is_same<::std::remove_cv_t<T>, __uint128_t>
#endif
                                           >;

template <typename T>
inline constexpr bool is_cpp_integral_v = is_cpp_integral<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CppIntegral = is_cpp_integral_v<T>;

#endif

// Detect C++ FP types.
template <typename T>
using is_cpp_floating_point = ::std::is_floating_point<T>;

template <typename T>
inline constexpr bool is_cpp_floating_point_v = is_cpp_floating_point<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CppFloatingPoint = is_cpp_floating_point_v<T>;

#endif

// Detect C++ arithmetic types, including GCC-style 128bit integers.
template <typename T>
using is_cpp_arithmetic = ::std::disjunction<is_cpp_integral<T>, is_cpp_floating_point<T>>;

template <typename T>
inline constexpr bool is_cpp_arithmetic_v = is_cpp_arithmetic<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CppArithmetic = is_cpp_arithmetic_v<T>;

#endif

template <typename T>
using is_const = ::std::is_const<T>;

template <typename T>
inline constexpr bool is_const_v = is_const<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Const = is_const_v<T>;

#endif

template <typename T>
using is_signed = ::std::disjunction<::std::is_signed<T>
#if defined(PIRANHA_HAVE_GCC_INT128)
                                     ,
                                     ::std::is_same<::std::remove_cv_t<T>, __int128_t>
#endif
                                     >;

template <typename T>
inline constexpr bool is_signed_v = is_signed<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Signed = is_signed_v<T>;

#endif

namespace detail
{

template <typename T>
struct make_unsigned_impl : ::std::make_unsigned<T> {
};

#if defined(PIRANHA_HAVE_GCC_INT128)

// TODO fix.
template <>
struct make_unsigned_impl<__int128_t> {
    using type = __uint128_t;
};

#endif

} // namespace detail

template <typename T>
using make_unsigned_t = typename detail::make_unsigned_impl<T>::type;

namespace detail
{

// Small wrapper to fetch the min/max values of a builtin numerical type. Works on 128bit integrals as well.
template <typename T>
inline constexpr auto limits_minmax = ::std::tuple{::std::numeric_limits<T>::min(), ::std::numeric_limits<T>::max()};

#if defined(PIRANHA_HAVE_GCC_INT128)

inline constexpr auto max_int128_t = (((__int128_t(1) << 126) - 1) << 1) + 1;

template <>
inline constexpr auto limits_minmax<__int128_t> = ::std::tuple{-max_int128_t - 1, max_int128_t};

template <>
inline constexpr auto limits_minmax<__uint128_t> = ::std::tuple{__uint128_t(0), ~__uint128_t(0)};

#endif

} // namespace detail

} // namespace piranha

#endif