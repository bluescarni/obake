// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/config.hpp>
#include <piranha/utils/bit_packing.hpp>

namespace piranha::detail
{

namespace
{

template <typename T>
constexpr auto mmsp = compute_minmax_packed<T>();

}

template <>
const decltype(compute_minmax_packed<int>()) foo_cont<int>::value = mmsp<int>;

#if 0

template <typename T>
const decltype(compute_minmax_packed<T>()) foo_cont<T>::value = mmsp<T>;

template struct foo_cont<int>;
template struct foo_cont<long>;
template struct foo_cont<long long>;

#if defined(PIRANHA_HAVE_GCC_INT128)

template struct foo_cont<__int128_t>;

#endif

#endif

} // namespace piranha::detail
