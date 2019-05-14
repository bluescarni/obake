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
constexpr auto sbp_compute_minmax_packed()
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

// Use a constexpr wrapper for the computation
// in order to ensure the values are calculated at compile time.
// This allows us to check at compile time that we are not
// incurring in undefined behaviour due to excessive
// shifting, etc.
template <typename T>
constexpr auto sbp_mmp_impl = detail::sbp_compute_minmax_packed<T>();

// Same as above, but for unsigned types.
template <typename T>
constexpr auto ubp_compute_max_packed()
{
    constexpr auto nbits = static_cast<unsigned>(limits_digits<T>);

    // Init the return value.
    ubp_max_packed_t<T> retval{};

    for (auto i = 0u; i < retval.size(); ++i) {
        const auto size = i + 1u;
        // The maximum decodable value is a sequence of
        // N one bits (starting from LSB), where N is the
        // largest multiple of size fitting in nbits.
        retval[i] = T(-1) >> (nbits % size);
    }

    return retval;
}

template <typename T>
constexpr auto ubp_max_impl = detail::ubp_compute_max_packed<T>();

} // namespace

// Init the constants with the constexpr-computed values.
// NOTE: because sbp_minmax_packed_t has a constexpr ctor, and because we use
// constexpr variables for initialisation, these values will be precomputed at compile
// time and they are guaranteed to be available before the dynamic initialisation
// of other global variables.
const sbp_minmax_packed_t<int> sbp_mmp_int = sbp_mmp_impl<int>;
const sbp_minmax_packed_t<long> sbp_mmp_long = sbp_mmp_impl<long>;
const sbp_minmax_packed_t<long long> sbp_mmp_long_long = sbp_mmp_impl<long long>;

#if defined(PIRANHA_HAVE_GCC_INT128)

const sbp_minmax_packed_t<__int128_t> sbp_mmp_int128 = sbp_mmp_impl<__int128_t>;

#endif

const ubp_max_packed_t<unsigned> ubp_max_unsigned = ubp_max_impl<unsigned>;
const ubp_max_packed_t<unsigned long> ubp_max_ulong = ubp_max_impl<unsigned long>;
const ubp_max_packed_t<unsigned long long> ubp_max_ulonglong = ubp_max_impl<unsigned long long>;

#if defined(PIRANHA_HAVE_GCC_INT128)

const ubp_max_packed_t<__uint128_t> ubp_max_uint128 = ubp_max_impl<__uint128_t>;

#endif

} // namespace piranha::detail
