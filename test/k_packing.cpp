// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <random>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/k_packing.hpp>
#include <obake/type_name.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

using int_types = std::tuple<int, unsigned, long, unsigned long, long long, unsigned long long
// NOTE: clang + ubsan fail to compile with 128bit integers in this test.
#if defined(OBAKE_HAVE_GCC_INT128) && !defined(OBAKE_TEST_CLANG_UBSAN)
                             ,
                             __int128_t, __uint128_t
#endif
                             >;

static std::mt19937 rng;

constexpr auto ntrials = 100;

#if defined(_MSC_VER)

#pragma warning(push)
#pragma warning(disable : 4146)
#pragma warning(disable : 4307)

#endif

TEST_CASE("k_packer_unpacker")
{
    obake_test::disable_slow_stack_traces();

    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using kp_t = k_packer<int_t>;
        using ku_t = k_unpacker<int_t>;

        const auto lim_min = detail::limits_min<int_t>;
        const auto lim_max = detail::limits_max<int_t>;

        constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<int_t>);

        // Start with an empty packer.
        kp_t kp0(0);
        REQUIRE(kp0.get() == int_t(0));

        // Check that adding a value to the packer throws.
        OBAKE_REQUIRES_THROWS_CONTAINS(
            kp0 << int_t{0}, std::out_of_range,
            "Cannot push any more values to this Kronecker packer for the type '" + type_name<int_t>()
                + "': the number of "
                  "values already pushed to the packer is equal to the size used for construction (0)");

        // Empty unpacker.
        ku_t ku0(0, 0);
        int_t out;
        OBAKE_REQUIRES_THROWS_CONTAINS(ku0 >> out, std::out_of_range,
                                       "Cannot unpack any more values from this Kronecker unpacker: the number of "
                                       "values already unpacked is equal to the size used for construction (0)");

        // Empty unpacker with nonzero value.
        OBAKE_REQUIRES_THROWS_CONTAINS(ku_t(42, 0), std::invalid_argument,
                                       "Only a value of zero can be used in a Kronecker unpacker with a size of zero, "
                                       "but a value of 42 was provided instead");

        // Test the error thrown if we try to init an unpacker whose size is too large.
        OBAKE_REQUIRES_THROWS_CONTAINS(
            ku_t(0, nbits / 3u + 1u), std::overflow_error,
            "Invalid size specified in the constructor of a Kronecker unpacker for the type '");
        OBAKE_REQUIRES_THROWS_CONTAINS(ku_t(0, nbits / 3u + 1u), std::overflow_error,
                                       "': the maximum possible size is " + detail::to_string(nbits / 3u)
                                           + ", but a size of " + detail::to_string(nbits / 3u + 1u)
                                           + " was specified instead");

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
#if defined(OBAKE_HAVE_GCC_INT128)
        if constexpr (!std::is_same_v<int_t, __int128_t> && !std::is_same_v<int_t, __uint128_t>)
#endif
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
#if defined(OBAKE_HAVE_GCC_INT128)
        if constexpr (!std::is_same_v<int_t, __int128_t> && !std::is_same_v<int_t, __uint128_t>)
#endif
        {
            for (auto size = 2u; size <= nbits / 3u; ++size) {
                // Number of bits corresponding to the current size.
                const auto cur_nb = static_cast<unsigned>(detail::limits_digits<int_t>) / size;

                // Get the components' limits for the current number of bits.
                const auto &lims = std::get<2>(detail::k_packing_data<int_t>)[cur_nb - 3u];

                std::vector<int_t> v(size);
                std::uniform_int_distribution<int_t> idist;

                for (auto k = 0; k < ntrials; ++k) {
                    kp1 = kp_t(size);
                    REQUIRE(kp1.get() == int_t{0});
                    for (auto j = 0u; j < size; ++j) {
                        if constexpr (is_signed_v<int_t>) {
                            v[j] = idist(
                                rng, typename std::uniform_int_distribution<int_t>::param_type{lims[j][0], lims[j][1]});
                        } else {
                            v[j] = idist(rng,
                                         typename std::uniform_int_distribution<int_t>::param_type{int_t(0), lims[j]});
                        }
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
                    OBAKE_REQUIRES_THROWS_CONTAINS(kp1 << (lims[0][1] + int_t(1)), std::overflow_error,
                                                   "Cannot push the value " + detail::to_string(lims[0][1] + int_t(1))
                                                       + " to this Kronecker packer for the type '" + type_name<int_t>()
                                                       + "': the value is outside the allowed range ["
                                                       + detail::to_string(lims[0][0]) + ", "
                                                       + detail::to_string(lims[0][1]) + "]");
                    OBAKE_REQUIRES_THROWS_CONTAINS(kp1 << (lims[0][0] - int_t(1)), std::overflow_error,
                                                   "Cannot push the value " + detail::to_string(lims[0][0] - int_t(1))
                                                       + " to this Kronecker packer for the type '" + type_name<int_t>()
                                                       + "': the value is outside the allowed range ["
                                                       + detail::to_string(lims[0][0]) + ", "
                                                       + detail::to_string(lims[0][1]) + "]");
                } else {
                    OBAKE_REQUIRES_THROWS_CONTAINS(kp1 << (lims[0] + int_t(1)), std::overflow_error,
                                                   "Cannot push the value " + detail::to_string(lims[0] + int_t(1))
                                                       + " to this Kronecker packer for the type '" + type_name<int_t>()
                                                       + "': the value is outside the allowed range [0, "
                                                       + detail::to_string(lims[0]) + "]");
                }

                // Check out of range unpacking.
                const auto &e_lim = std::get<3>(detail::k_packing_data<int_t>)[nbits / 3u - size];

                if constexpr (is_signed_v<int_t>) {
                    if (e_lim[0] > lim_min) {
                        OBAKE_REQUIRES_THROWS_CONTAINS(ku_t(e_lim[0] - int_t(1), size), std::overflow_error,
                                                       "The value " + detail::to_string(e_lim[0] - int_t(1))
                                                           + " passed to a Kronecker unpacker of size "
                                                           + detail::to_string(size) + " is outside the allowed range ["
                                                           + detail::to_string(e_lim[0]) + ", "
                                                           + detail::to_string(e_lim[1]) + "]");
                    }
                    if (e_lim[1] < lim_max) {
                        OBAKE_REQUIRES_THROWS_CONTAINS(ku_t(e_lim[1] + int_t(1), size), std::overflow_error,
                                                       "The value " + detail::to_string(e_lim[1] + int_t(1))
                                                           + " passed to a Kronecker unpacker of size "
                                                           + detail::to_string(size) + " is outside the allowed range ["
                                                           + detail::to_string(e_lim[0]) + ", "
                                                           + detail::to_string(e_lim[1]) + "]");
                    }
                } else {
                    if (e_lim < lim_max) {
                        OBAKE_REQUIRES_THROWS_CONTAINS(
                            ku_t(e_lim + int_t(1), size), std::overflow_error,
                            "The value " + detail::to_string(e_lim + int_t(1))
                                + " passed to a Kronecker unpacker of size " + detail::to_string(size)
                                + " is outside the allowed range [0, " + detail::to_string(e_lim) + "]");
                    }
                }

                // Test maximal/minimal packing.
                if constexpr (is_signed_v<int_t>) {
                    kp1 = kp_t(size);
                    for (auto j = 0u; j < size; ++j) {
                        v[j] = lims[j][0];
                        kp1 << lims[j][0];
                    }
                    ku1 = ku_t(kp1.get(), size);
                    for (const auto &x : v) {
                        ku1 >> out;
                        REQUIRE(out == x);
                    }

                    kp1 = kp_t(size);
                    for (auto j = 0u; j < size; ++j) {
                        v[j] = lims[j][1];
                        kp1 << lims[j][1];
                    }
                    ku1 = ku_t(kp1.get(), size);
                    for (const auto &x : v) {
                        ku1 >> out;
                        REQUIRE(out == x);
                    }
                } else {
                    kp1 = kp_t(size);
                    for (auto j = 0u; j < size; ++j) {
                        v[j] = lims[j];
                        kp1 << lims[j];
                    }
                    ku1 = ku_t(kp1.get(), size);
                    for (const auto &x : v) {
                        ku1 >> out;
                        REQUIRE(out == x);
                    }
                }
            }
        }

        // Some additional error checking.
        OBAKE_REQUIRES_THROWS_CONTAINS(kp_t(nbits / 3u + 1u), std::overflow_error,
                                       "Invalid size specified in the constructor of a Kronecker packer for the type '"
                                           + type_name<int_t>() + "': the maximum possible size is "
                                           + detail::to_string(nbits / 3u) + ", but a size of "
                                           + detail::to_string(nbits / 3u + 1u) + " was specified instead");

        kp1 = kp_t(3);
        kp1 << int_t(0) << int_t(0) << int_t(0);
        OBAKE_REQUIRES_THROWS_CONTAINS(
            kp1 << int_t(0), std::out_of_range,
            "Cannot push any more values to this Kronecker packer for the type '" + type_name<int_t>()
                + "': the number of "
                  "values already pushed to the packer is equal to the size used for construction (3)");
    });
}

TEST_CASE("homomorphism")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
#if defined(OBAKE_HAVE_GCC_INT128)
        if constexpr (std::is_same_v<int_t, __int128_t> || std::is_same_v<int_t, __uint128_t>) {
            return;
        } else {
#endif
            using kp_t = k_packer<int_t>;

            static constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<int_t>);

            for (auto i = 1u; i <= nbits / 3u; ++i) {
                // Number of bits corresponding to the current size.
                const auto cur_nb = nbits / i;

                std::vector<int_t> a(i), b(i), c(i);
                std::uniform_int_distribution<int_t> idist;
                for (auto k = 0; k < ntrials; ++k) {
                    kp_t kp_a(i), kp_b(i), kp_c(i);
                    for (auto j = 0u; j < i; ++j) {
                        if (i == 1u) {
                            a[j] = idist(
                                rng, typename std::uniform_int_distribution<int_t>::param_type{
                                         detail::limits_min<int_t> / int_t(2), detail::limits_max<int_t> / int_t(2)});
                            b[j] = idist(
                                rng, typename std::uniform_int_distribution<int_t>::param_type{
                                         detail::limits_min<int_t> / int_t(2), detail::limits_max<int_t> / int_t(2)});
                        } else {
                            const auto &lims = std::get<2>(detail::k_packing_data<int_t>)[cur_nb - 3u];

                            if constexpr (is_signed_v<int_t>) {
                                a[j] = idist(rng, typename std::uniform_int_distribution<int_t>::param_type{
                                                      lims[j][0] / int_t(2), lims[j][1] / int_t(2)});
                                b[j] = idist(rng, typename std::uniform_int_distribution<int_t>::param_type{
                                                      lims[j][0] / int_t(2), lims[j][1] / int_t(2)});
                            } else {
                                a[j] = idist(rng, typename std::uniform_int_distribution<int_t>::param_type{
                                                      int_t(0), lims[j] / int_t(2)});
                                b[j] = idist(rng, typename std::uniform_int_distribution<int_t>::param_type{
                                                      int_t(0), lims[j] / int_t(2)});
                            }
                        }
                        c[j] = a[j] + b[j];
                        kp_a << a[j];
                        kp_b << b[j];
                        kp_c << c[j];
                    }
                    REQUIRE(kp_a.get() + kp_b.get() == kp_c.get());
                }
            }
#if defined(OBAKE_HAVE_GCC_INT128)
        }
#endif
    });
}

#if defined(_MSC_VER)

#pragma warning(pop)

#endif
