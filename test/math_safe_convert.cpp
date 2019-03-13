// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/math/safe_convert.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <limits>

#include <piranha/config.hpp>
#include <piranha/type_traits.hpp>

#include <mp++/integer.hpp>

TEST_CASE("safe_convert_integrals")
{
    int n;
    unsigned u;

    // Same signedness, same types. Conversion will always succeed.
    REQUIRE(piranha::safe_convert(n, 45));
    REQUIRE(n == 45);
    REQUIRE(piranha::safe_convert(n, -45));
    REQUIRE(n == -45);
    REQUIRE(piranha::safe_convert(u, 41u));
    REQUIRE(u == 41u);

    // Same width, but signed vs unsigned.
    REQUIRE(piranha::safe_convert(n, 55u));
    REQUIRE(n == 55);
    REQUIRE(piranha::safe_convert(u, 54));
    REQUIRE(u == 54u);
    REQUIRE(!piranha::safe_convert(u, -54));
    REQUIRE(u == 54u);

    // Different types with different ranges.
    if constexpr (std::numeric_limits<int>::max() < std::numeric_limits<long>::max()
                  && std::numeric_limits<unsigned>::max() < std::numeric_limits<unsigned long>::max()) {
        long l;
        unsigned long ul;

        // Int to long, success.
        REQUIRE(piranha::safe_convert(l, 42));
        REQUIRE(l == 42);
        // Long to int, success.
        REQUIRE(piranha::safe_convert(n, 40l));
        REQUIRE(n == 40);
        // Long to int, failure.
        REQUIRE(!piranha::safe_convert(n, std::numeric_limits<long>::max()));
        REQUIRE(n == 40);

        // Unsigned to ulong, success.
        REQUIRE(piranha::safe_convert(ul, 38u));
        REQUIRE(ul == 38u);
        // Ulong to unsigned, success.
        REQUIRE(piranha::safe_convert(u, 36ul));
        REQUIRE(u == 36u);
        // Ulong to unsigned, failure.
        REQUIRE(!piranha::safe_convert(u, std::numeric_limits<unsigned long>::max()));
        REQUIRE(u == 36u);
    }

    // Couple of tests with bool.
    bool b;
    REQUIRE(piranha::safe_convert(b, 0));
    REQUIRE(!b);
    REQUIRE(piranha::safe_convert(b, 1));
    REQUIRE(b);
    REQUIRE(!piranha::safe_convert(b, -1));
    REQUIRE(b);
    REQUIRE(!piranha::safe_convert(b, 2));
    REQUIRE(b);
    REQUIRE(piranha::safe_convert(b, 0));
    REQUIRE(!b);
    REQUIRE(piranha::safe_convert(n, true));
    REQUIRE(n == 1);
    REQUIRE(piranha::safe_convert(n, false));
    REQUIRE(n == 0);

#if defined(PIRANHA_HAVE_GCC_INT128)

    __int128_t i128;
    __uint128_t ui128;
    REQUIRE(piranha::safe_convert(i128, 45));
    REQUIRE(i128 == 45);
    REQUIRE(piranha::safe_convert(i128, -45));
    REQUIRE(i128 == -45);
    REQUIRE(piranha::safe_convert(ui128, 45));
    REQUIRE(ui128 == 45u);
    REQUIRE(!piranha::safe_convert(ui128, -44));
    REQUIRE(ui128 == 45u);
    REQUIRE(piranha::safe_convert(n, __int128_t{31}));
    REQUIRE(n == 31);
    REQUIRE(piranha::safe_convert(n, __uint128_t{31}));
    REQUIRE(n == 31);
    REQUIRE(piranha::safe_convert(n, __int128_t{-31}));
    REQUIRE(n == -31);
    REQUIRE(piranha::safe_convert(u, __uint128_t{30}));
    REQUIRE(u == 30u);
    REQUIRE(piranha::safe_convert(u, __int128_t{30}));
    REQUIRE(u == 30u);
    REQUIRE(!piranha::safe_convert(u, __int128_t{-30}));
    REQUIRE(u == 30u);

    if constexpr (std::get<1>(piranha::detail::limits_minmax<__int128_t>) > std::numeric_limits<int>::max()
                  && std::get<1>(piranha::detail::limits_minmax<__uint128_t>) > std::numeric_limits<unsigned>::max()) {
        REQUIRE(!piranha::safe_convert(n, std::get<1>(piranha::detail::limits_minmax<__int128_t>)));
        REQUIRE(!piranha::safe_convert(u, std::get<1>(piranha::detail::limits_minmax<__uint128_t>)));
    }

#endif
}

TEST_CASE("safe_convert_mppp_integer")
{
    using int_t = mppp::integer<1>;

    int n;
    REQUIRE(piranha::safe_convert(n, int_t{10}));
    REQUIRE(n == 10);
    REQUIRE(piranha::safe_convert(n, int_t{-10}));
    REQUIRE(n == -10);
    REQUIRE(!piranha::safe_convert(n, int_t{std::numeric_limits<int>::max()} + 1));
    REQUIRE(n == -10);
    REQUIRE(!piranha::safe_convert(n, int_t{std::numeric_limits<int>::min()} - 1));
    REQUIRE(n == -10);

    int_t out;
    REQUIRE(piranha::safe_convert(out, 10));
    REQUIRE(out == 10);
    REQUIRE(piranha::safe_convert(out, -10000l));
    REQUIRE(out == -10000l);

#if defined(PIRANHA_HAVE_GCC_INT128)

    __int128_t i128;
    __uint128_t ui128;
    REQUIRE(piranha::safe_convert(i128, int_t{45}));
    REQUIRE(i128 == 45);
    REQUIRE(piranha::safe_convert(i128, int_t{-45}));
    REQUIRE(i128 == -45);
    REQUIRE(piranha::safe_convert(ui128, int_t{45}));
    REQUIRE(ui128 == 45u);
    REQUIRE(!piranha::safe_convert(ui128, int_t{-44}));
    REQUIRE(ui128 == 45u);
    REQUIRE(piranha::safe_convert(out, __int128_t{33}));
    REQUIRE(out == 33);
    REQUIRE(piranha::safe_convert(out, __int128_t{-33}));
    REQUIRE(out == -33);
    REQUIRE(piranha::safe_convert(out, __uint128_t{32}));
    REQUIRE(out == 32);
    REQUIRE(!piranha::safe_convert(i128, int_t{std::get<1>(piranha::detail::limits_minmax<__int128_t>)} + 1));
    REQUIRE(!piranha::safe_convert(ui128, int_t{std::get<1>(piranha::detail::limits_minmax<__uint128_t>)} + 1));

#endif
}
