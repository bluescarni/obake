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
auto minmax_signed_packed<int> = mmsp<int>;

template <>
auto minmax_signed_packed<long> = mmsp<long>;

template <>
auto minmax_signed_packed<long long> = mmsp<long long>;

#if defined(PIRANHA_HAVE_GCC_INT128)

template <>
auto minmax_signed_packed<__int128_t> = mmsp<__int128_t>;

#endif

} // namespace piranha::detail
