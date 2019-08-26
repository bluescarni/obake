// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
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

#include <piranha/config.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/type_traits.hpp>

#include "catch.hpp"

// Verify constexpr capability.
constexpr auto c_iz = piranha::is_zero(0);

template <bool>
struct foo_b {
};

[[maybe_unused]] static foo_b<piranha::is_zero(1)> fb;

TEST_CASE("is_zero_arith")
{
    REQUIRE(!piranha::is_zero_testable_v<void>);
    REQUIRE(piranha::is_zero_testable_v<float>);
    REQUIRE(piranha::is_zero_testable_v<int>);
    REQUIRE(piranha::is_zero_testable_v<double &&>);
    REQUIRE(piranha::is_zero_testable_v<const long double>);
    REQUIRE(piranha::is_zero_testable_v<short &>);
    REQUIRE(piranha::is_zero_testable_v<const char &>);

#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::is_zero_testable_v<__int128_t>);
    REQUIRE(piranha::is_zero_testable_v<__uint128_t>);
    REQUIRE(piranha::is_zero_testable_v<__int128_t &&>);
    REQUIRE(piranha::is_zero_testable_v<const __uint128_t>);
    REQUIRE(piranha::is_zero_testable_v<const __uint128_t &>);
    REQUIRE(piranha::is_zero_testable_v<__int128_t &>);
#endif
#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!piranha::ZeroTestable<void>);
    REQUIRE(piranha::ZeroTestable<float>);
    REQUIRE(piranha::ZeroTestable<int>);
    REQUIRE(piranha::ZeroTestable<double &&>);
    REQUIRE(piranha::ZeroTestable<const long double>);
    REQUIRE(piranha::ZeroTestable<short &>);
    REQUIRE(piranha::ZeroTestable<const char &>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::ZeroTestable<__int128_t>);
    REQUIRE(piranha::ZeroTestable<__uint128_t>);
    REQUIRE(piranha::ZeroTestable<__int128_t &&>);
    REQUIRE(piranha::ZeroTestable<const __uint128_t>);
    REQUIRE(piranha::ZeroTestable<const __uint128_t &>);
    REQUIRE(piranha::ZeroTestable<__int128_t &>);
#endif
#endif

    REQUIRE(c_iz);
    REQUIRE(piranha::is_zero(0));
    REQUIRE(piranha::is_zero(0u));
    REQUIRE(piranha::is_zero(short(0)));
    REQUIRE(piranha::is_zero(0.));
    REQUIRE(piranha::is_zero(0.f));
    REQUIRE(!piranha::is_zero(1.f));
    REQUIRE(!piranha::is_zero(-1));
    REQUIRE(!piranha::is_zero(42ll));
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::is_zero(__int128_t(0)));
    REQUIRE(piranha::is_zero(__uint128_t(0)));
    REQUIRE(!piranha::is_zero(__uint128_t(42)));
    REQUIRE(!piranha::is_zero(__int128_t(-42)));
#endif

    if constexpr (std::numeric_limits<double>::has_infinity && std::numeric_limits<double>::has_quiet_NaN) {
        REQUIRE(!piranha::is_zero(std::numeric_limits<double>::infinity()));
        REQUIRE(!piranha::is_zero(std::numeric_limits<double>::quiet_NaN()));
    }
}

TEST_CASE("is_zero_mp++_int")
{
    using int_t = mppp::integer<1>;

    REQUIRE(piranha::is_zero_testable_v<int_t>);
    REQUIRE(piranha::is_zero_testable_v<int_t &>);
    REQUIRE(piranha::is_zero_testable_v<const int_t &>);
    REQUIRE(piranha::is_zero_testable_v<int_t &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::ZeroTestable<int_t>);
    REQUIRE(piranha::ZeroTestable<int_t &>);
    REQUIRE(piranha::ZeroTestable<const int_t &>);
    REQUIRE(piranha::ZeroTestable<int_t &&>);
#endif

    REQUIRE(piranha::is_zero(int_t{}));
    REQUIRE(!piranha::is_zero(int_t{-1}));
}

TEST_CASE("is_zero_mp++_rat")
{
    using rat_t = mppp::rational<1>;

    REQUIRE(piranha::is_zero_testable_v<rat_t>);
    REQUIRE(piranha::is_zero_testable_v<rat_t &>);
    REQUIRE(piranha::is_zero_testable_v<const rat_t &>);
    REQUIRE(piranha::is_zero_testable_v<rat_t &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::ZeroTestable<rat_t>);
    REQUIRE(piranha::ZeroTestable<rat_t &>);
    REQUIRE(piranha::ZeroTestable<const rat_t &>);
    REQUIRE(piranha::ZeroTestable<rat_t &&>);
#endif

    REQUIRE(piranha::is_zero(rat_t{}));
    REQUIRE(!piranha::is_zero(rat_t{-1, 45}));
}

#if defined(MPPP_WITH_MPFR)

TEST_CASE("is_zero_mp++_real")
{
    REQUIRE(piranha::is_zero_testable_v<mppp::real>);
    REQUIRE(piranha::is_zero_testable_v<mppp::real &>);
    REQUIRE(piranha::is_zero_testable_v<const mppp::real &>);
    REQUIRE(piranha::is_zero_testable_v<mppp::real &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::ZeroTestable<mppp::real>);
    REQUIRE(piranha::ZeroTestable<mppp::real &>);
    REQUIRE(piranha::ZeroTestable<const mppp::real &>);
    REQUIRE(piranha::ZeroTestable<mppp::real &&>);
#endif

    REQUIRE(piranha::is_zero(mppp::real{}));
    REQUIRE(!piranha::is_zero(mppp::real{42}));
    REQUIRE(!piranha::is_zero(mppp::real{"inf", 100}));
    REQUIRE(!piranha::is_zero(mppp::real{"nan", 100}));
}

#endif

#if defined(MPPP_WITH_QUADMATH)

TEST_CASE("is_zero_mp++_real128")
{
    REQUIRE(piranha::is_zero_testable_v<mppp::real128>);
    REQUIRE(piranha::is_zero_testable_v<mppp::real128 &>);
    REQUIRE(piranha::is_zero_testable_v<const mppp::real128 &>);
    REQUIRE(piranha::is_zero_testable_v<mppp::real128 &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::ZeroTestable<mppp::real128>);
    REQUIRE(piranha::ZeroTestable<mppp::real128 &>);
    REQUIRE(piranha::ZeroTestable<const mppp::real128 &>);
    REQUIRE(piranha::ZeroTestable<mppp::real128 &&>);
#endif

    REQUIRE(piranha::is_zero(mppp::real128{}));
    REQUIRE(!piranha::is_zero(mppp::real128{42}));
    REQUIRE(!piranha::is_zero(mppp::real128{"inf"}));
    REQUIRE(!piranha::is_zero(mppp::real128{"nan"}));

    // Verify constexpr.
    [[maybe_unused]] foo_b<piranha::is_zero(mppp::real128{})> tmp;
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

namespace piranha::customisation
{

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, is_zero_1> inline constexpr auto is_zero<T>
#else
inline constexpr auto is_zero<T, std::enable_if_t<is_same_cvr_v<T, is_zero_1>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return true;
};

} // namespace piranha::customisation

TEST_CASE("is_zero_custom")
{
    // Check type-traits/concepts.
    REQUIRE(!piranha::is_zero_testable_v<no_is_zero_0>);
    REQUIRE(!piranha::is_zero_testable_v<no_is_zero_1>);
    REQUIRE(piranha::is_zero_testable_v<is_zero_0>);
    REQUIRE(piranha::is_zero_testable_v<is_zero_1>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!piranha::ZeroTestable<no_is_zero_0>);
    REQUIRE(!piranha::ZeroTestable<no_is_zero_1>);
    REQUIRE(piranha::ZeroTestable<is_zero_0>);
    REQUIRE(piranha::ZeroTestable<is_zero_1>);
#endif
}
