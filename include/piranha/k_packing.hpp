// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_K_PACKING_HPP
#define PIRANHA_K_PACKING_HPP

#include <type_traits>

#include <piranha/config.hpp>

namespace piranha
{

// Only allow certain integral types to be packable (this is due to the complications arising
// from integral promotion rules for short ints and char types).
template <typename T>
using is_k_packable = ::std::disjunction<::std::is_same<T, int>, ::std::is_same<T, unsigned>, ::std::is_same<T, long>,
                                         ::std::is_same<T, unsigned long>, ::std::is_same<T, long long>,
                                         ::std::is_same<T, unsigned long long>
#if defined(PIRANHA_HAVE_GCC_INT128)
                                         ,
                                         ::std::is_same<T, __int128_t>, ::std::is_same<T, __uint128_t>
#endif
                                         >;

template <typename T>
inline constexpr bool is_k_packable_v = is_k_packable<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL KPackable = is_k_packable_v<T>;

#endif

namespace detail
{

template <typename T>
class unsigned_k_packer_impl
{
};

} // namespace detail

} // namespace piranha

#endif
