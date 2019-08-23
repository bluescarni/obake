// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <random>
#include <stdexcept>
#include <tuple>

#include <piranha/config.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/detail/tuple_for_each.hpp>
#include <piranha/k_packing.hpp>
#include <piranha/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace piranha;

using int_types = std::tuple<int, unsigned, long, unsigned long, long long, unsigned long long
#if defined(PIRANHA_HAVE_GCC_INT128)
                             ,
                             __int128_t, __uint128_t
#endif
                             >;

static std::mt19937 rng;

constexpr auto ntrials = 100;

#if defined(_MSC_VER)

#pragma warning(push)
#pragma warning(disable : 4146)

#endif

TEST_CASE("k_packer_unpacker")
{
    piranha_test::disable_slow_stack_traces();

    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using kp_t = k_packer<int_t>;
        using ku_t = k_unpacker<int_t>;

        using Catch::Matchers::Contains;

        const auto [lim_min, lim_max] = detail::limits_minmax<int_t>;

        constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<int_t>);

        // Start with an empty packer.
        kp_t kp0(0);
        REQUIRE(kp0.get() == int_t(0));

        // Check that adding a value to the packer throws.
        REQUIRE_THROWS_WITH(
            kp0 << int_t{0},
            Contains(
                "the number of values already pushed to the packer is equal to the size used for construction (0)"));
        REQUIRE_THROWS_AS(kp0 << int_t{0}, std::out_of_range);

        // Empty unpacker.
        ku_t ku0(0, 0);
        int_t out;
        REQUIRE_THROWS_WITH(
            ku0 >> out,
            Contains("the number of values already unpacked is equal to the size used for construction (0)"));
        REQUIRE_THROWS_AS(ku0 >> out, std::out_of_range);

        // Empty unpacker with nonzero value.
        REQUIRE_THROWS_WITH(ku_t(42, 0),
                            Contains("Only a value of zero can be used in a Kronecker unpacker with a size of zero, "
                                     "but a value of 42 was provided instead"));
        REQUIRE_THROWS_AS(ku_t(42, 0), std::invalid_argument);

        // Test the error thrown if we try to init an unpacker whose size is too large.
        REQUIRE_THROWS_WITH(
            ku_t(0, nbits / 3u + 1u),
            Contains("Invalid size specified in the constructor of a Kronecker unpacker for the type '"));
        REQUIRE_THROWS_WITH(ku_t(0, nbits / 3u + 1u),
                            Contains("': the maximum possible size is " + detail::to_string(nbits / 3u)
                                     + ", but a size of " + detail::to_string(nbits / 3u + 1u)
                                     + " was specified instead"));
        REQUIRE_THROWS_AS(ku_t(0, nbits / 3u + 1u), std::overflow_error);

        // Unitary packing/unpacking.
        kp_t kp1(1);
        REQUIRE_THROWS_WITH(
            kp1.get(),
            Contains("the number of values pushed to the packer (0) is less than the size used for construction (1)"));
        REQUIRE_THROWS_AS(kp1.get(), std::out_of_range);

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
    });
}
