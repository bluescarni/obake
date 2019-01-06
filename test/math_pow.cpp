// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/math/pow.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cmath>
#include <string>
#include <type_traits>
#include <utility>

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
#include <piranha/type_traits.hpp>

TEST_CASE("pow_arith")
{
    // Check type-traits/concepts.
    REQUIRE(piranha::is_exponentiable_v<float, int>);
    REQUIRE(piranha::is_exponentiable_v<double, float>);
    REQUIRE(piranha::is_exponentiable_v<const double &, long &&>);
    REQUIRE(piranha::is_exponentiable_v<int &&, const double>);
    REQUIRE(!piranha::is_exponentiable_v<float, std::string>);
    REQUIRE(!piranha::is_exponentiable_v<std::string, float>);
    REQUIRE(!piranha::is_exponentiable_v<int, int>);
    REQUIRE(!piranha::is_exponentiable_v<float, void>);
    REQUIRE(!piranha::is_exponentiable_v<void, float>);
    REQUIRE(!piranha::is_exponentiable_v<void, void>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::is_exponentiable_v<float, __int128_t>);
    REQUIRE(piranha::is_exponentiable_v<float, __uint128_t>);
    REQUIRE(piranha::is_exponentiable_v<__int128_t &&, const double>);
    REQUIRE(piranha::is_exponentiable_v<__uint128_t &&, const double>);
    REQUIRE(!piranha::is_exponentiable_v<__uint128_t &&, __int128_t>);
    REQUIRE(!piranha::is_exponentiable_v<__int128_t &&, __uint128_t>);
#endif
#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::Exponentiable<float, int>);
    REQUIRE(piranha::Exponentiable<const double &, float &&>);
    REQUIRE(piranha::Exponentiable<long double &, float &&>);
    REQUIRE(!piranha::Exponentiable<int, const bool &>);
    REQUIRE(!piranha::Exponentiable<short, unsigned char &&>);
    REQUIRE(!piranha::Exponentiable<double, void>);
    REQUIRE(!piranha::Exponentiable<void, double>);
    REQUIRE(!piranha::Exponentiable<void, void>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::Exponentiable<float, __int128_t>);
    REQUIRE(piranha::Exponentiable<__uint128_t, long double &>);
    REQUIRE(!piranha::Exponentiable<int, const __int128_t>);
    REQUIRE(!piranha::Exponentiable<__uint128_t, const __int128_t>);
#endif
#endif

    // Simple checks, and check the return types.
    REQUIRE(piranha::pow(3., 5) == std::pow(3., 5));
    REQUIRE(piranha::pow(5, 3.) == std::pow(5, 3.));
    REQUIRE(piranha::pow(3., -2.) == std::pow(3., -2.));
    REQUIRE(piranha::pow(3.f, -2.f) == std::pow(3.f, -2.f));
    REQUIRE(piranha::pow(3.l, -2.l) == std::pow(3.l, -2.l));
    REQUIRE(std::is_same_v<decltype(piranha::pow(3., 5)), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(5, 3.)), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.f, 5)), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(5, 3.f)), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.l, 5)), long double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(5, 3.l)), long double>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::pow(3., __int128_t(5)) == std::pow(3., 5));
    REQUIRE(piranha::pow(__uint128_t(5), 3.) == std::pow(5, 3.));
    REQUIRE(std::is_same_v<decltype(piranha::pow(3., __int128_t(5))), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3., __uint128_t(5))), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__int128_t(5), 3.)), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__uint128_t(5), 3.)), double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.f, __int128_t(5))), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.f, __uint128_t(5))), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__int128_t(5), 3.f)), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__uint128_t(5), 3.f)), float>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.l, __int128_t(5))), long double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(3.l, __uint128_t(5))), long double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__int128_t(5), 3.l)), long double>);
    REQUIRE(std::is_same_v<decltype(piranha::pow(__uint128_t(5), 3.l)), long double>);
#endif

    // Check perfect forwarding.
    REQUIRE(noexcept(piranha::pow(1.5, 1.5)));
    REQUIRE(noexcept(piranha::pow(1.5, 1)));
    REQUIRE(noexcept(piranha::pow(1, 1.5)));
    REQUIRE(noexcept(piranha::pow(1.5f, 1.5f)));
    REQUIRE(noexcept(piranha::pow(1.5f, 1)));
    REQUIRE(noexcept(piranha::pow(1, 1.5f)));
}

TEST_CASE("pow_mp++_int")
{
    using int_t = mppp::integer<1>;

    // A few simple checks.
    REQUIRE(piranha::pow(int_t{3}, 5) == 243);
    REQUIRE(piranha::pow(3, int_t{5}) == 243);
    REQUIRE(piranha::pow(3., int_t{5}) == std::pow(3., 5.));
    REQUIRE(piranha::pow(int_t{5}, 3.) == std::pow(5., 3.));
    REQUIRE(!noexcept(piranha::pow(int_t{5}, 3.)));
#if defined(MPPP_WITH_MPFR)
    REQUIRE(piranha::pow(3.l, int_t{5}) == std::pow(3.l, 5.l));
    REQUIRE(piranha::pow(int_t{5}, 3.l) == std::pow(5.l, 3.l));
#else
    REQUIRE(!piranha::is_exponentiable_v<long double, int_t>);
    REQUIRE(!piranha::is_exponentiable_v<int_t, long double>);
#endif
    REQUIRE(!piranha::is_exponentiable_v<int_t, std::string>);
}

TEST_CASE("pow_mp++_rat")
{
    using rat_t = mppp::rational<1>;

    // A few simple checks.
    REQUIRE(piranha::pow(rat_t{3, 2}, 5) == rat_t{243, 32});
    REQUIRE(piranha::pow(3., rat_t{5, 2}) == std::pow(3., 5. / 2.));
    REQUIRE(piranha::pow(rat_t{5, 2}, 3.) == std::pow(5. / 2., 3.));
    REQUIRE(!noexcept(piranha::pow(rat_t{5}, 3.)));
#if defined(MPPP_WITH_MPFR)
    REQUIRE(piranha::pow(3.l, rat_t{5}) == std::pow(3.l, 5.l));
    REQUIRE(piranha::pow(rat_t{5}, 3.l) == std::pow(5.l, 3.l));
#else
    REQUIRE(!piranha::is_exponentiable_v<long double, rat_t>);
    REQUIRE(!piranha::is_exponentiable_v<rat_t, long double>);
#endif
    REQUIRE(!piranha::is_exponentiable_v<rat_t, std::string>);
}

#if defined(MPPP_WITH_MPFR)

TEST_CASE("pow_mp++_real")
{
    using mppp::real;

    // A few simple checks.
    REQUIRE(piranha::pow(real{3}, 5) == 243);
    REQUIRE(piranha::pow(3, real{5}) == 243);
    REQUIRE(piranha::pow(3., real{5}) == std::pow(3., 5.));
    REQUIRE(piranha::pow(real{5}, 3.) == std::pow(5., 3.));
    REQUIRE(!noexcept(piranha::pow(real{5}, 3.)));
    REQUIRE(!piranha::is_exponentiable_v<real, std::string>);
}

#endif

#if defined(MPPP_WITH_QUADMATH)

TEST_CASE("pow_mp++_real")
{
    using mppp::real128;

    // A few simple checks.
    REQUIRE(piranha::pow(real128{3}, 5) == 243);
    REQUIRE(piranha::pow(3, real128{5}) == 243);
    REQUIRE(piranha::pow(3., real128{5}) == std::pow(3., 5.));
    REQUIRE(piranha::pow(real128{5}, 3.) == std::pow(5., 3.));
    REQUIRE(!noexcept(piranha::pow(real128{5}, 3.)));
    REQUIRE(!piranha::is_exponentiable_v<real128, std::string>);
}

#endif

// Test the customisation machinery.

// A new type.
struct foo0 {
};

// A non-constexpr function.
inline void non_constexpr() {}

// Customise piranha::pow() for foo0.
namespace piranha::customisation
{

template <typename T, typename U>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvref<T, foo0> &&SameCvref<U, foo0> inline constexpr auto pow<T, U>
#else
inline constexpr auto pow<T, U, std::enable_if_t<is_same_cvref_v<T, foo0> && is_same_cvref_v<U, foo0>>>
#endif
    = [](auto &&, auto &&) constexpr noexcept
{
    if constexpr (std::is_rvalue_reference_v<T> && std::is_rvalue_reference_v<U>) {
        return 1;
    } else {
        // Make some non-constexpr operation.
        non_constexpr();
        return 2;
    }
};

} // namespace piranha::customisation

template <int>
struct bar {
};

TEST_CASE("pow_custom")
{
    REQUIRE(!piranha::is_exponentiable_v<foo0, int>);
    REQUIRE(!piranha::is_exponentiable_v<int, foo0>);
    REQUIRE(piranha::is_exponentiable_v<foo0, foo0>);

    REQUIRE(piranha::pow(foo0{}, foo0{}) == 1);
    foo0 f;
    REQUIRE(piranha::pow(f, foo0{}) == 2);
    REQUIRE(piranha::pow(foo0{}, f) == 2);
    REQUIRE(piranha::pow(std::move(f), foo0{}) == 1);
    REQUIRE(piranha::pow(foo0{}, std::move(f)) == 1);

    // Check that constexpr is preserved if only rvalues are involved.
    [[maybe_unused]] bar<piranha::pow(foo0{}, foo0{})> b0;
    // This will result in a compilation error.
    // [[maybe_unused]] bar<piranha::pow(foo0{}, f)> b1;
}
