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

const minmax_packed_t<int> mmp_int = mmsp<int>;
const minmax_packed_t<long> mmp_long = mmsp<long>;
const minmax_packed_t<long long> mmp_long_long = mmsp<long long>;

#if defined(PIRANHA_HAVE_GCC_INT128)

const minmax_packed_t<__int128_t> mmp_int128 = mmsp<__int128_t>;

#endif

} // namespace piranha::detail
