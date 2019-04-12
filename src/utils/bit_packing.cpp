// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cassert>
#include <tuple>

#include <piranha/config.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/utils/bit_packing.hpp>

namespace piranha::detail
{

namespace
{

// Helper to compute the min/max packed values for a signed integral T
// and for all the possible packer sizes.
template <typename T>
constexpr sbp_minmax_packed_t<T> sbp_compute_minmax_packed()
{
    constexpr auto nbits = static_cast<unsigned>(limits_digits<T> + 1);

    // Init the return value.
    sbp_minmax_packed_t<T> retval{};

    // For size 1, we have the special case of using the full range.
    retval[0][0] = ::std::get<0>(limits_minmax<T>);
    retval[0][1] = ::std::get<1>(limits_minmax<T>);

    // Build the remaining sizes.
    for (auto i = 1u; i < retval.size(); ++i) {
        // Pack vectors of min/max values for this size.
        const auto size = i + 1u;
        bit_packer<T> bp_min(size), bp_max(size);
        const auto pbits = nbits / size - static_cast<unsigned>(nbits % size == 0u);
        assert(pbits);
        const auto min = -(T(1) << (pbits - 1u)), max = (T(1) << (pbits - 1u)) - T(1);
        for (auto j = 0u; j < size; ++j) {
            bp_min << min;
            bp_max << max;
        }
        // Extract the packed values.
        retval[i][0] = bp_min.get();
        retval[i][1] = bp_max.get();
    }

    return retval;
}

} // namespace

// Init the constants.
// NOTE: because sbp_minmax_packed_t has a constexpr ctor, and because we use a
// constexpr function for initialisation, these values will be precomputed at compile
// time and they are guaranteed to be available before the dynamic initialisation
// of other global variables.
const sbp_minmax_packed_t<int> sbp_mmp_int = detail::sbp_compute_minmax_packed<int>();
const sbp_minmax_packed_t<long> sbp_mmp_long = detail::sbp_compute_minmax_packed<long>();
const sbp_minmax_packed_t<long long> sbp_mmp_long_long = detail::sbp_compute_minmax_packed<long long>();

#if defined(PIRANHA_HAVE_GCC_INT128)

const sbp_minmax_packed_t<__int128_t> sbp_mmp_int128 = detail::sbp_compute_minmax_packed<__int128_t>();

#endif

} // namespace piranha::detail
