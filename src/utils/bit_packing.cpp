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

// Use a constexpr wrapper for the computation
// in order to ensure the values are calculated at compile time.
// This allows us to check at compile time that we are not
// incurring in undefined behaviour due to excessive
// shifting, etc.
// As usual, put the local names in an unnamed namespace
// in order to preempt ODR violations.
template <typename T>
constexpr auto sbp_mmp_impl = detail::sbp_compute_minmax_packed<T>();

} // namespace

// Init the constants with the constexpr-computed values.
const sbp_minmax_packed_t<int> sbp_mmp_int = sbp_mmp_impl<int>;
const sbp_minmax_packed_t<long> sbp_mmp_long = sbp_mmp_impl<long>;
const sbp_minmax_packed_t<long long> sbp_mmp_long_long = sbp_mmp_impl<long long>;

#if defined(PIRANHA_HAVE_GCC_INT128)

const sbp_minmax_packed_t<__int128_t> sbp_mmp_int128 = sbp_mmp_impl<__int128_t>;

#endif

} // namespace piranha::detail
