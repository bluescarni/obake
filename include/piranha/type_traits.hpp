// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_TYPE_TRAITS_HPP
#define PIRANHA_TYPE_TRAITS_HPP

#include <type_traits>

#include <piranha/config.hpp>

namespace piranha
{

template <typename T>
inline constexpr bool is_cpp_integral
    = std::disjunction_v<std::is_integral<T>
#if defined(PIRANHA_HAVE_GCC_INT128)
                         ,
                         std::is_same<std::remove_cv_t<T>, __int128_t>, std::is_same<std::remove_cv_t<T>, __uint128_t>
#endif
                         >;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CppIntegral = ::piranha::is_cpp_integral<T>;

#endif

template <typename T>
inline constexpr bool is_cpp_floating_point = std::is_floating_point_v<T>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CppFloatingPoint = ::piranha::is_cpp_floating_point<T>;

#endif

} // namespace piranha

#endif