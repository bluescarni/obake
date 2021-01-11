// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <array>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include <obake/config.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/kpack.hpp>
#include <obake/type_name.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

using int_types = std::tuple<std::int32_t, std::uint32_t
#if defined(OBAKE_PACKABLE_INT64)
                             ,
                             std::int64_t, std::uint64_t
#endif
                             >;

static std::mt19937 rng;

constexpr auto ntrials = 10000;

TEST_CASE("k_packer_unpacker")
{
    obake_test::disable_slow_stack_traces();

    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using kp_t = kpacker<int_t>;
        using ku_t = kunpacker<int_t>;

        // Limits for unitary packing/unpacking.
        const auto [lim_min, lim_max] = detail::kpack_get_lims<int_t>(1);

        // Start with an empty packer.
        kp_t kp0(0);
        REQUIRE(kp0.get() == int_t(0));

        // Check that adding a value to the packer throws.
        OBAKE_REQUIRES_THROWS_CONTAINS(kp0 << int_t{0}, std::out_of_range,
                                       "Cannot push any more values to this Kronecker packer for the type '"
                                           + type_name<int_t>()
                                           + "': the number of "
                                             "values already pushed to the packer is equal to the packer's size (0)");

        // Empty unpacker.
        ku_t ku0(0, 0);
        int_t out;
        OBAKE_REQUIRES_THROWS_CONTAINS(ku0 >> out, std::out_of_range,
                                       "Cannot unpack any more values from this Kronecker unpacker: the number of "
                                       "values already unpacked is equal to the unpacker's size (0)");

        // Empty unpacker with nonzero value.
        OBAKE_REQUIRES_THROWS_CONTAINS(ku_t(42, 0), std::invalid_argument,
                                       "Only a value of zero can be used in a Kronecker unpacker with a size of zero, "
                                       "but a value of 42 was provided instead");

        // Test the error thrown if we try to init an unpacker whose size is too large.
        OBAKE_REQUIRES_THROWS_CONTAINS(
            ku_t(0, detail::kpack_max_size<int_t>() + 1u), std::overflow_error,
            "Invalid size specified in the constructor of a Kronecker unpacker for the type '");
        OBAKE_REQUIRES_THROWS_CONTAINS(
            ku_t(0, detail::kpack_max_size<int_t>() + 1u), std::overflow_error,
            "': the maximum possible size is " + detail::to_string(detail::kpack_max_size<int_t>()) + ", but a size of "
                + detail::to_string(detail::kpack_max_size<int_t>() + 1u) + " was specified instead");

        // Unitary packing/unpacking.
        kp_t kp1(1);
        REQUIRE(kp1.get() == int_t{0});

        // Try the limits.
        kp1 << lim_min;
        ku_t ku1(kp1.get(), 1);
        ku1 >> out;
        REQUIRE(out == lim_min);

        kp1 = kp_t(1);
        kp1 << lim_max;
        ku1 = ku_t(kp1.get(), 1);
        ku1 >> out;
        REQUIRE(out == lim_max);

        // Random testing.
        {
            std::uniform_int_distribution<int_t> idist(lim_min, lim_max);
            for (auto i = 0; i < ntrials; ++i) {
                const auto tmp = idist(rng);
                kp1 = kp_t(1);
                kp1 << tmp;
                ku1 = ku_t(kp1.get(), 1);
                ku1 >> out;
                REQUIRE(tmp == out);
            }
        }

        // Random testing with variable sizes.
        {
            for (auto size = 2u; size <= detail::kpack_max_size<int_t>(); ++size) {
                // Get the components' limits for the current number of bits.
                const auto lims
                    = std::array{detail::kpack_get_lims<int_t>(size).first, detail::kpack_get_lims<int_t>(size).second};

                std::vector<int_t> v(size);
                std::uniform_int_distribution<int_t> idist;

                for (auto k = 0; k < ntrials; ++k) {
                    kp1 = kp_t(size);
                    REQUIRE(kp1.get() == int_t{0});
                    for (auto j = 0u; j < size; ++j) {
                        v[j] = idist(rng, typename std::uniform_int_distribution<int_t>::param_type{lims[0], lims[1]});
                        kp1 << v[j];
                    }
                    ku1 = ku_t(kp1.get(), size);
                    for (const auto &x : v) {
                        ku1 >> out;
                        REQUIRE(out == x);
                    }
                }

                // Check that packing zeroes gives a zero value.
                kp1 = kp_t(size);
                for (auto j = 0u; j < size; ++j) {
                    kp1 << int_t(0);
                }
                REQUIRE(kp1.get() == int_t(0));
                ku1 = ku_t(kp1.get(), size);
                for (auto j = 0u; j < size; ++j) {
                    ku1 >> out;
                    REQUIRE(out == int_t(0));
                }

                // Check out of range packing.
                kp1 = kp_t(size);
                if constexpr (is_signed_v<int_t>) {
                    OBAKE_REQUIRES_THROWS_CONTAINS(kp1 << (lims[1] + int_t(1)), std::overflow_error,
                                                   "Cannot push the value " + detail::to_string(lims[1] + int_t(1))
                                                       + " to this Kronecker packer for the type '" + type_name<int_t>()
                                                       + "': the value is outside the allowed range ["
                                                       + detail::to_string(lims[0]) + ", " + detail::to_string(lims[1])
                                                       + "]");
                    OBAKE_REQUIRES_THROWS_CONTAINS(kp1 << (lims[0] - int_t(1)), std::overflow_error,
                                                   "Cannot push the value " + detail::to_string(lims[0] - int_t(1))
                                                       + " to this Kronecker packer for the type '" + type_name<int_t>()
                                                       + "': the value is outside the allowed range ["
                                                       + detail::to_string(lims[0]) + ", " + detail::to_string(lims[1])
                                                       + "]");
                } else {
                    OBAKE_REQUIRES_THROWS_CONTAINS(kp1 << (lims[1] + int_t(1)), std::overflow_error,
                                                   "Cannot push the value " + detail::to_string(lims[1] + int_t(1))
                                                       + " to this Kronecker packer for the type '" + type_name<int_t>()
                                                       + "': the value is outside the allowed range [0, "
                                                       + detail::to_string(lims[1]) + "]");
                }

                // Check out of range unpacking.
                const auto [klim_min, klim_max] = detail::kpack_get_klims<int_t>(size);

                if constexpr (is_signed_v<int_t>) {
                    OBAKE_REQUIRES_THROWS_CONTAINS(ku_t(klim_min - int_t(1), size), std::overflow_error,
                                                   "The value " + detail::to_string(klim_min - int_t(1))
                                                       + " passed to a Kronecker unpacker for the type '"
                                                       + type_name<int_t>() + "' is outside the allowed range ["
                                                       + detail::to_string(klim_min) + ", "
                                                       + detail::to_string(klim_max) + "]");
                    OBAKE_REQUIRES_THROWS_CONTAINS(ku_t(klim_max + int_t(1), size), std::overflow_error,
                                                   "The value " + detail::to_string(klim_max + int_t(1))
                                                       + " passed to a Kronecker unpacker for the type '"
                                                       + type_name<int_t>() + "' is outside the allowed range ["
                                                       + detail::to_string(klim_min) + ", "
                                                       + detail::to_string(klim_max) + "]");

                } else {
                    OBAKE_REQUIRES_THROWS_CONTAINS(ku_t(klim_max + int_t(1), size), std::overflow_error,
                                                   "The value " + detail::to_string(klim_max + int_t(1))
                                                       + " passed to a Kronecker unpacker for the type '"
                                                       + type_name<int_t>() + "' is outside the allowed range [0, "
                                                       + detail::to_string(klim_max) + "]");
                }

                // Test maximal/minimal packing.
                kp1 = kp_t(size);
                for (auto j = 0u; j < size; ++j) {
                    v[j] = lims[0];
                    kp1 << lims[0];
                }
                ku1 = ku_t(kp1.get(), size);
                for (const auto &x : v) {
                    ku1 >> out;
                    REQUIRE(out == x);
                }

                kp1 = kp_t(size);
                for (auto j = 0u; j < size; ++j) {
                    v[j] = lims[1];
                    kp1 << lims[1];
                }
                ku1 = ku_t(kp1.get(), size);
                for (const auto &x : v) {
                    ku1 >> out;
                    REQUIRE(out == x);
                }
            }
        }

        // Some additional error checking.
        OBAKE_REQUIRES_THROWS_CONTAINS(kp_t(detail::kpack_max_size<int_t>() + 1u), std::overflow_error,
                                       "Invalid size specified in the constructor of a Kronecker packer for the type '"
                                           + type_name<int_t>() + "': the maximum possible size is "
                                           + detail::to_string(detail::kpack_max_size<int_t>()) + ", but a size of "
                                           + detail::to_string(detail::kpack_max_size<int_t>() + 1u)
                                           + " was specified instead");

        kp1 = kp_t(3);
        kp1 << int_t(0) << int_t(0) << int_t(0);
        OBAKE_REQUIRES_THROWS_CONTAINS(
            kp1 << int_t(0), std::out_of_range,
            "Cannot push any more values to this Kronecker packer for the type '" + type_name<int_t>()
                + "': the number of values already pushed to the packer is equal to the packer's size (3)");
    });
}
