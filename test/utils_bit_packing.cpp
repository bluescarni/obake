// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/utils/bit_packing.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#include <piranha/config.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/detail/tuple_for_each.hpp>
#include <piranha/type_traits.hpp>

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

TEST_CASE("bit_packer_unpacker")
{
    piranha_test::disable_slow_stack_traces();

    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using bp_t = bit_packer<int_t>;
        using bu_t = bit_unpacker<int_t>;

        using Catch::Matchers::Contains;

        const auto [lim_min, lim_max] = detail::limits_minmax<int_t>;
        constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<int_t>);

        // Start with an empty packer.
        bp_t bp0(0);
        REQUIRE(bp0.get() == int_t(0));

        // Check that adding a value to the packer throws.
        REQUIRE_THROWS_WITH(
            bp0 << int_t{0},
            Contains(
                "the number of values already pushed to the packer is equal to the size used for construction (0)"));
        REQUIRE_THROWS_AS(bp0 << int_t{0}, std::out_of_range);

        // Empty unpacker.
        bu_t bu0(0, 0);
        int_t out;
        REQUIRE_THROWS_WITH(
            bu0 >> out,
            Contains("the number of values already unpacked is equal to the size used for construction (0)"));
        REQUIRE_THROWS_AS(bu0 >> out, std::out_of_range);

        // Empty unpacker with nonzero value.
        REQUIRE_THROWS_WITH(bu_t(42, 0), Contains("Only a value of zero can be unpacked into an empty output range, "
                                                  "but a value of 42 was provided instead"));
        REQUIRE_THROWS_AS(bu_t(42, 0), ::std::invalid_argument);

        // Test the error thrown if we try to init an unpacker whose size is too large.
        if constexpr (is_signed_v<int_t>) {
            REQUIRE_THROWS_WITH(
                bu_t(0, nbits + 1u),
                Contains("The size of a signed bit unpacker must be smaller than the bit width of the integral type ("
                         + detail::to_string(nbits + 1u) + "), but a size of " + detail::to_string(nbits + 1u)
                         + " was specified"));
            REQUIRE_THROWS_AS(bu_t(0, nbits + 1u), std::overflow_error);
        } else {
            REQUIRE_THROWS_WITH(
                bu_t(0, nbits + 1u),
                Contains(
                    "The size of an unsigned bit unpacker cannot be larger than the bit width of the integral type ("
                    + detail::to_string(nbits) + "), but a size of " + detail::to_string(nbits + 1u)
                    + " was specified"));
            REQUIRE_THROWS_AS(bu_t(0, nbits + 1u), std::overflow_error);
        }

        // Unitary packing/unpacking.
        bp_t bp1(1);
        REQUIRE_THROWS_WITH(
            bp1.get(),
            Contains("the number of values pushed to the packer (0) is less than the size used for construction (1)"));
        REQUIRE_THROWS_AS(bp1.get(), std::out_of_range);

        // Try the limits.
        bp1 << lim_min;
        bu_t bu1(bp1.get(), 1);
        bu1 >> out;
        REQUIRE(out == lim_min);

        bp1 = bp_t(1);
        bp1 << lim_max;
        bu1 = bu_t(bp1.get(), 1);
        bu1 >> out;
        REQUIRE(out == lim_max);

        // Random testing.
#if defined(PIRANHA_HAVE_GCC_INT128)
        if constexpr (!std::is_same_v<int_t, __int128_t> && !std::is_same_v<int_t, __uint128_t>)
#endif
        {
            std::uniform_int_distribution<int_t> idist(lim_min, lim_max);
            for (auto i = 0; i < ntrials; ++i) {
                const auto tmp = idist(rng);
                bp1 = bp_t(1);
                bp1 << tmp;
                bu1 = bu_t(bp1.get(), 1);
                bu1 >> out;
                REQUIRE(tmp == out);
            }
        }

        // Random testing with variable sizes.
#if defined(PIRANHA_HAVE_GCC_INT128)
        if constexpr (!std::is_same_v<int_t, __int128_t> && !std::is_same_v<int_t, __uint128_t>)
#endif
        {
            if constexpr (is_signed_v<int_t>) {
                for (auto i = 2u; i <= nbits; ++i) {
                    const auto pbits = (nbits + 1u) / i - static_cast<unsigned>((nbits + 1u) % i == 0u);
                    // Compute the limits.
                    const auto cur_min = -(int_t(1) << (pbits - 1u)), cur_max = (int_t(1) << (pbits - 1u)) - int_t(1);
                    std::uniform_int_distribution<int_t> idist(cur_min, cur_max);
                    std::vector<int_t> v(i);
                    for (auto k = 0; k < ntrials; ++k) {
                        bp1 = bp_t(i);
                        for (auto &x : v) {
                            x = idist(rng);
                            bp1 << x;
                        }
                        bu1 = bu_t(bp1.get(), i);
                        for (const auto &x : v) {
                            bu1 >> out;
                            REQUIRE(out == x);
                        }
                    }

                    // Check out of range packing.
                    bp1 = bp_t(i);
                    REQUIRE_THROWS_WITH(
                        bp1 << (cur_max + int_t(1)),
                        Contains("Cannot push the value " + detail::to_string(cur_max + int_t(1))
                                 + " to this signed bit packer: the value is outside the allowed range ["
                                 + detail::to_string(cur_min) + ", " + detail::to_string(cur_max) + "]"));
                    REQUIRE_THROWS_AS(bp1 << (cur_max + int_t(1)), std::overflow_error);
                    REQUIRE_THROWS_WITH(
                        bp1 << (cur_min - int_t(1)),
                        Contains("Cannot push the value " + detail::to_string(cur_min - int_t(1))
                                 + " to this signed bit packer: the value is outside the allowed range ["
                                 + detail::to_string(cur_min) + ", " + detail::to_string(cur_max) + "]"));
                    REQUIRE_THROWS_AS(bp1 << (cur_min - int_t(1)), std::overflow_error);

                    const auto [min_dec, max_dec] = detail::sbp_get_mmp<int_t>()[i - 1u];

                    REQUIRE_THROWS_WITH(bu_t(max_dec + int_t(1), i),
                                        Contains("The value " + detail::to_string(max_dec + int_t(1))
                                                 + " passed to a signed bit unpacker of size " + detail::to_string(i)
                                                 + " is outside the allowed range [" + detail::to_string(min_dec) + ", "
                                                 + detail::to_string(max_dec) + "]"));
                    REQUIRE_THROWS_AS(bu_t(max_dec + int_t(1), i), std::overflow_error);

                    REQUIRE_THROWS_WITH(bu_t(min_dec - int_t(1), i),
                                        Contains("The value " + detail::to_string(min_dec - int_t(1))
                                                 + " passed to a signed bit unpacker of size " + detail::to_string(i)
                                                 + " is outside the allowed range [" + detail::to_string(min_dec) + ", "
                                                 + detail::to_string(max_dec) + "]"));
                    REQUIRE_THROWS_AS(bu_t(min_dec - int_t(1), i), std::overflow_error);
                }
            } else {
                for (auto i = 2u; i <= nbits; ++i) {
                    const auto pbits = nbits / i;
                    const auto cur_max = (int_t(1) << pbits) - int_t(1);
                    std::uniform_int_distribution<int_t> idist(int_t(0), cur_max);
                    std::vector<int_t> v(i);
                    for (auto k = 0; k < ntrials; ++k) {
                        bp1 = bp_t(i);
                        for (auto &x : v) {
                            x = idist(rng);
                            bp1 << x;
                        }
                        bu1 = bu_t(bp1.get(), i);
                        for (const auto &x : v) {
                            bu1 >> out;
                            REQUIRE(out == x);
                        }
                    }

                    // Check out of range packing.
                    bp1 = bp_t(i);
                    REQUIRE_THROWS_WITH(
                        bp1 << (cur_max + int_t(1)),
                        Contains("Cannot push the value " + detail::to_string(cur_max + int_t(1))
                                 + " to this unsigned bit packer: the value is outside the allowed range [0, "
                                 + detail::to_string(cur_max) + "]"));
                    REQUIRE_THROWS_AS(bp1 << (cur_max + int_t(1)), std::overflow_error);

                    // If the current size does not divide nbits exactly, we can
                    // construct a value which is larger than the max decodable value.
                    if (nbits % i) {
                        const auto max_decodable = int_t(-1) >> (nbits % i);

                        REQUIRE_THROWS_WITH(bu_t(max_decodable + 1u, i),
                                            Contains("The value " + detail::to_string(max_decodable + 1u)
                                                     + " passed to an unsigned bit unpacker of size "
                                                     + detail::to_string(i) + " is outside the allowed range [0, "
                                                     + detail::to_string(max_decodable) + "]"));
                        REQUIRE_THROWS_AS(bu_t(max_decodable + 1u, i), std::overflow_error);

                        REQUIRE_THROWS_WITH(bu_t(max_decodable + 2u, i),
                                            Contains("The value " + detail::to_string(max_decodable + 2u)
                                                     + " passed to an unsigned bit unpacker of size "
                                                     + detail::to_string(i) + " is outside the allowed range [0, "
                                                     + detail::to_string(max_decodable) + "]"));
                        REQUIRE_THROWS_AS(bu_t(max_decodable + 2u, i), std::overflow_error);
                    }
                }
            }
        }

        // Error checking.
        if constexpr (is_signed_v<int_t>) {
            REQUIRE_THROWS_WITH(
                bp_t(nbits + 1u),
                Contains("The size of a signed bit packer must be smaller than the bit width of the integral type ("
                         + detail::to_string(nbits + 1u) + "), but a size of " + detail::to_string(nbits + 1u)
                         + " was specified"));
            REQUIRE_THROWS_AS(bp_t(nbits + 1u), std::overflow_error);

            bp1 = bp_t(3);
            bp1 << int_t(0) << int_t(0) << int_t(0);
            REQUIRE_THROWS_WITH(
                bp1 << int_t(0),
                Contains("Cannot push any more values to this signed bit packer: the number of "
                         "values already pushed to the packer is equal to the size used for construction (3)"));
            REQUIRE_THROWS_AS(bp1 << int_t(0), std::out_of_range);
        } else {
            REQUIRE_THROWS_WITH(
                bp_t(nbits + 1u),
                Contains(
                    "The size of an unsigned bit packer must not be larger than the bit width of the integral type ("
                    + detail::to_string(nbits) + "), but a size of " + detail::to_string(nbits + 1u)
                    + " was specified"));
            REQUIRE_THROWS_AS(bp_t(nbits + 1u), std::overflow_error);

            bp1 = bp_t(3);
            bp1 << int_t(0) << int_t(0) << int_t(0);
            REQUIRE_THROWS_WITH(
                bp1 << int_t(0),
                Contains("Cannot push any more values to this unsigned bit packer: the number of "
                         "values already pushed to the packer is equal to the size used for construction (3)"));
            REQUIRE_THROWS_AS(bp1 << int_t(0), std::out_of_range);
        }
    });
}

TEST_CASE("homomorphism")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
#if defined(PIRANHA_HAVE_GCC_INT128)
        if constexpr (std::is_same_v<int_t, __int128_t> || std::is_same_v<int_t, __uint128_t>) {
            return;
        } else {
#endif
            using bp_t = bit_packer<int_t>;

            constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<int_t>);

            for (auto i = 1u; i <= nbits; ++i) {
                std::vector<int_t> a(i), b(i), c(i);
                const auto [cur_min, cur_max] = [i]() {
                    if constexpr (is_signed_v<int_t>) {
                        if (i == 1u) {
                            return std::tuple{std::get<0>(detail::limits_minmax<int_t>) / int_t(2),
                                              std::get<1>(detail::limits_minmax<int_t>) / int_t(2)};
                        } else {
                            const auto pbits = (nbits + 1u) / i - static_cast<unsigned>((nbits + 1u) % i == 0u);
                            return std::tuple{-(int_t(1) << (pbits - 1u)) / int_t(2),
                                              ((int_t(1) << (pbits - 1u)) - int_t(1)) / int_t(2)};
                        }
                    } else {
                        if (i == 1u) {
                            return std::tuple{int_t(0), int_t(-1) / int_t(2)};
                        } else {
                            const auto pbits = nbits / i;
                            return std::tuple{int_t(0), ((int_t(1) << pbits) - int_t(1)) / int_t(2)};
                        }
                    }
                }();
                std::uniform_int_distribution<int_t> idist(cur_min, cur_max);
                for (auto k = 0; k < ntrials; ++k) {
                    bp_t bp_a(i), bp_b(i), bp_c(i);
                    for (auto j = 0u; j < i; ++j) {
                        a[j] = idist(rng);
                        b[j] = idist(rng);
                        c[j] = a[j] + b[j];
                        bp_a << a[j];
                        bp_b << b[j];
                        bp_c << c[j];
                    }
                    REQUIRE(bp_a.get() + bp_b.get() == bp_c.get());
                }
            }
#if defined(PIRANHA_HAVE_GCC_INT128)
        }
#endif
    });
}

#if defined(_MSC_VER)

#pragma warning(pop)

#endif
