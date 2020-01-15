// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <limits>
#include <type_traits>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>
#if defined(MPPP_WITH_MPFR)
#include <mp++/real.hpp>
#endif
#if defined(MPPP_WITH_QUADMATH)
#include <mp++/real128.hpp>
#endif

#include <obake/config.hpp>
#include <obake/math/is_zero.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

// Verify constexpr capability.
constexpr auto c_iz = obake::is_zero(0);

template <bool>
struct foo_b {
};

[[maybe_unused]] static foo_b<obake::is_zero(1)> fb;

TEST_CASE("is_zero_arith")
{
    REQUIRE(!obake::is_zero_testable_v<void>);
    REQUIRE(obake::is_zero_testable_v<float>);
    REQUIRE(obake::is_zero_testable_v<int>);
    REQUIRE(obake::is_zero_testable_v<double &&>);
    REQUIRE(obake::is_zero_testable_v<const long double>);
    REQUIRE(obake::is_zero_testable_v<short &>);
    REQUIRE(obake::is_zero_testable_v<const char &>);

#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::is_zero_testable_v<__int128_t>);
    REQUIRE(obake::is_zero_testable_v<__uint128_t>);
    REQUIRE(obake::is_zero_testable_v<__int128_t &&>);
    REQUIRE(obake::is_zero_testable_v<const __uint128_t>);
    REQUIRE(obake::is_zero_testable_v<const __uint128_t &>);
    REQUIRE(obake::is_zero_testable_v<__int128_t &>);
#endif
#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!obake::ZeroTestable<void>);
    REQUIRE(obake::ZeroTestable<float>);
    REQUIRE(obake::ZeroTestable<int>);
    REQUIRE(obake::ZeroTestable<double &&>);
    REQUIRE(obake::ZeroTestable<const long double>);
    REQUIRE(obake::ZeroTestable<short &>);
    REQUIRE(obake::ZeroTestable<const char &>);
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::ZeroTestable<__int128_t>);
    REQUIRE(obake::ZeroTestable<__uint128_t>);
    REQUIRE(obake::ZeroTestable<__int128_t &&>);
    REQUIRE(obake::ZeroTestable<const __uint128_t>);
    REQUIRE(obake::ZeroTestable<const __uint128_t &>);
    REQUIRE(obake::ZeroTestable<__int128_t &>);
#endif
#endif

    REQUIRE(c_iz);
    REQUIRE(obake::is_zero(0));
    REQUIRE(obake::is_zero(0u));
    REQUIRE(obake::is_zero(short(0)));
    REQUIRE(obake::is_zero(0.));
    REQUIRE(obake::is_zero(0.f));
    REQUIRE(!obake::is_zero(1.f));
    REQUIRE(!obake::is_zero(-1));
    REQUIRE(!obake::is_zero(42ll));
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::is_zero(__int128_t(0)));
    REQUIRE(obake::is_zero(__uint128_t(0)));
    REQUIRE(!obake::is_zero(__uint128_t(42)));
    REQUIRE(!obake::is_zero(__int128_t(-42)));
#endif

    if constexpr (std::numeric_limits<double>::has_infinity && std::numeric_limits<double>::has_quiet_NaN) {
        REQUIRE(!obake::is_zero(std::numeric_limits<double>::infinity()));
        REQUIRE(!obake::is_zero(std::numeric_limits<double>::quiet_NaN()));
    }
}

TEST_CASE("is_zero_mp++_int")
{
    using int_t = mppp::integer<1>;

    REQUIRE(obake::is_zero_testable_v<int_t>);
    REQUIRE(obake::is_zero_testable_v<int_t &>);
    REQUIRE(obake::is_zero_testable_v<const int_t &>);
    REQUIRE(obake::is_zero_testable_v<int_t &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::ZeroTestable<int_t>);
    REQUIRE(obake::ZeroTestable<int_t &>);
    REQUIRE(obake::ZeroTestable<const int_t &>);
    REQUIRE(obake::ZeroTestable<int_t &&>);
#endif

    REQUIRE(obake::is_zero(int_t{}));
    REQUIRE(!obake::is_zero(int_t{-1}));
}

TEST_CASE("is_zero_mp++_rat")
{
    using rat_t = mppp::rational<1>;

    REQUIRE(obake::is_zero_testable_v<rat_t>);
    REQUIRE(obake::is_zero_testable_v<rat_t &>);
    REQUIRE(obake::is_zero_testable_v<const rat_t &>);
    REQUIRE(obake::is_zero_testable_v<rat_t &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::ZeroTestable<rat_t>);
    REQUIRE(obake::ZeroTestable<rat_t &>);
    REQUIRE(obake::ZeroTestable<const rat_t &>);
    REQUIRE(obake::ZeroTestable<rat_t &&>);
#endif

    REQUIRE(obake::is_zero(rat_t{}));
    REQUIRE(!obake::is_zero(rat_t{-1, 45}));
}

#if defined(MPPP_WITH_MPFR)

TEST_CASE("is_zero_mp++_real")
{
    REQUIRE(obake::is_zero_testable_v<mppp::real>);
    REQUIRE(obake::is_zero_testable_v<mppp::real &>);
    REQUIRE(obake::is_zero_testable_v<const mppp::real &>);
    REQUIRE(obake::is_zero_testable_v<mppp::real &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::ZeroTestable<mppp::real>);
    REQUIRE(obake::ZeroTestable<mppp::real &>);
    REQUIRE(obake::ZeroTestable<const mppp::real &>);
    REQUIRE(obake::ZeroTestable<mppp::real &&>);
#endif

    REQUIRE(obake::is_zero(mppp::real{}));
    REQUIRE(!obake::is_zero(mppp::real{42}));
    REQUIRE(!obake::is_zero(mppp::real{"inf", 100}));
    REQUIRE(!obake::is_zero(mppp::real{"nan", 100}));
}

#endif

#if defined(MPPP_WITH_QUADMATH)

TEST_CASE("is_zero_mp++_real128")
{
    REQUIRE(obake::is_zero_testable_v<mppp::real128>);
    REQUIRE(obake::is_zero_testable_v<mppp::real128 &>);
    REQUIRE(obake::is_zero_testable_v<const mppp::real128 &>);
    REQUIRE(obake::is_zero_testable_v<mppp::real128 &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::ZeroTestable<mppp::real128>);
    REQUIRE(obake::ZeroTestable<mppp::real128 &>);
    REQUIRE(obake::ZeroTestable<const mppp::real128 &>);
    REQUIRE(obake::ZeroTestable<mppp::real128 &&>);
#endif

    REQUIRE(obake::is_zero(mppp::real128{}));
    REQUIRE(!obake::is_zero(mppp::real128{42}));
    REQUIRE(!obake::is_zero(mppp::real128{"inf"}));
    REQUIRE(!obake::is_zero(mppp::real128{"nan"}));

    // Verify constexpr.
    [[maybe_unused]] foo_b<obake::is_zero(mppp::real128{})> tmp;
}

#endif

struct no_is_zero_0 {
};

// Wrong return type in the ADL implementation.
struct no_is_zero_1 {
};

void is_zero(const no_is_zero_1 &);

// OK ADL implementation.
struct is_zero_0 {
};

int is_zero(const is_zero_0 &);

// External customisation point.
struct is_zero_1 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, is_zero_1> inline constexpr auto is_zero<T>
#else
inline constexpr auto is_zero<T, std::enable_if_t<is_same_cvr_v<T, is_zero_1>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return true;
};

} // namespace obake::customisation

TEST_CASE("is_zero_custom")
{
    // Check type-traits/concepts.
    REQUIRE(!obake::is_zero_testable_v<no_is_zero_0>);
    REQUIRE(!obake::is_zero_testable_v<no_is_zero_1>);
    REQUIRE(obake::is_zero_testable_v<is_zero_0>);
    REQUIRE(obake::is_zero_testable_v<is_zero_1>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!obake::ZeroTestable<no_is_zero_0>);
    REQUIRE(!obake::ZeroTestable<no_is_zero_1>);
    REQUIRE(obake::ZeroTestable<is_zero_0>);
    REQUIRE(obake::ZeroTestable<is_zero_1>);
#endif
}
