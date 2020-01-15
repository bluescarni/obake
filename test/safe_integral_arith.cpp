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
#include <utility>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/safe_integral_arith.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

std::mt19937 rng;

const int ntries = 1000;

using namespace obake;
using namespace obake::detail;

using int_types = std::tuple<char, unsigned char, signed char, short, unsigned short, int, unsigned, long,
                             unsigned long, long long, unsigned long long
#if defined(OBAKE_HAVE_GCC_INT128)
                             ,
                             __int128_t, __uint128_t
#endif
                             >;

TEST_CASE("add_test")
{
    obake_test::disable_slow_stack_traces();

    tuple_for_each(int_types{}, [](auto n) {
        using T = decltype(n);

        REQUIRE(safe_int_add(limits_max<T>, T(0)) == limits_max<T>);
        REQUIRE(safe_int_add(limits_min<T>, T(0)) == limits_min<T>);

        OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_add(limits_max<T>, T(1)), std::overflow_error,
                                       "Overflow error in an integral addition: ");
        OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_add(limits_max<T>, T(5)), std::overflow_error,
                                       "Overflow error in an integral addition: ");
        OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_add(limits_max<T>, T(50)), std::overflow_error,
                                       "Overflow error in an integral addition: ");
        if constexpr (is_signed_v<T>) {
            OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_add(limits_min<T>, T(-1)), std::overflow_error,
                                           "Overflow error in an integral addition: ");
            OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_add(limits_min<T>, T(-5)), std::overflow_error,
                                           "Overflow error in an integral addition: ");
            OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_add(limits_min<T>, T(-50)), std::overflow_error,
                                           "Overflow error in an integral addition: ");
        }

#if defined(OBAKE_HAVE_GCC_INT128)
        if constexpr (!std::is_same_v<T, __int128_t> && !std::is_same_v<T, __uint128_t>) {
#endif
            using r_type = decltype(T(0) + T(0));
            std::uniform_int_distribution<r_type> dist(limits_min<T> / T(5), limits_max<T> / T(5));
            for (auto i = 0; i < ntries; ++i) {
                const auto a = dist(rng), b = dist(rng);

                REQUIRE(safe_int_add(static_cast<T>(a), static_cast<T>(b)) == a + b);
            }
#if defined(OBAKE_HAVE_GCC_INT128)
        }
#endif
    });

    // Test booleans too.
    REQUIRE(!safe_int_add(false, false));
    REQUIRE(safe_int_add(true, false));
    REQUIRE(safe_int_add(false, true));
    OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_add(true, true), std::overflow_error,
                                   "Overflow error in an integral addition: ");
}

TEST_CASE("sub_test")
{
    tuple_for_each(int_types{}, [](auto n) {
        using T = decltype(n);

        REQUIRE(safe_int_sub(limits_max<T>, T(0)) == limits_max<T>);
        REQUIRE(safe_int_sub(limits_min<T>, T(0)) == limits_min<T>);

        OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_sub(limits_min<T>, T(1)), std::overflow_error,
                                       "Overflow error in an integral subtraction: ");
        OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_sub(limits_min<T>, T(5)), std::overflow_error,
                                       "Overflow error in an integral subtraction: ");
        OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_sub(limits_min<T>, T(50)), std::overflow_error,
                                       "Overflow error in an integral subtraction: ");

        if constexpr (is_signed_v<T>) {
            OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_sub(limits_max<T>, T(-1)), std::overflow_error,
                                           "Overflow error in an integral subtraction: ");
            OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_sub(limits_max<T>, T(-5)), std::overflow_error,
                                           "Overflow error in an integral subtraction: ");
            OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_sub(limits_max<T>, T(-50)), std::overflow_error,
                                           "Overflow error in an integral subtraction: ");
        }

#if defined(OBAKE_HAVE_GCC_INT128)
        if constexpr (!std::is_same_v<T, __int128_t> && !std::is_same_v<T, __uint128_t>) {
#endif
            using r_type = decltype(T(0) - T(0));
            std::uniform_int_distribution<r_type> dist(limits_min<T> / T(5), limits_max<T> / T(5));
            for (auto i = 0; i < ntries; ++i) {
                auto a = dist(rng), b = dist(rng);
                if (a < b && std::is_unsigned<T>::value) {
                    std::swap(a, b);
                }
                REQUIRE(safe_int_sub(static_cast<T>(a), static_cast<T>(b)) == a - b);
#if defined(OBAKE_HAVE_GCC_INT128)
            }
#endif
        }
    });

    // Test booleans too.
    REQUIRE(!safe_int_sub(false, false));
    REQUIRE(safe_int_sub(true, false));
    REQUIRE(!safe_int_sub(true, true));
    OBAKE_REQUIRES_THROWS_CONTAINS(safe_int_sub(false, true), std::overflow_error,
                                   "Overflow error in an integral subtraction: ");
}
