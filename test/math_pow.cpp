// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

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

#include <obake/config.hpp>
#include <obake/math/pow.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

TEST_CASE("pow_arith")
{
    // Check type-traits/concepts.
    REQUIRE(obake::is_exponentiable_v<float, int>);
    REQUIRE(obake::is_exponentiable_v<double, float>);
    REQUIRE(obake::is_exponentiable_v<const double &, long &&>);
    REQUIRE(obake::is_exponentiable_v<int &&, const double>);
    REQUIRE(!obake::is_exponentiable_v<float, std::string>);
    REQUIRE(!obake::is_exponentiable_v<std::string, float>);
    REQUIRE(!obake::is_exponentiable_v<int, int>);
    REQUIRE(!obake::is_exponentiable_v<float, void>);
    REQUIRE(!obake::is_exponentiable_v<void, float>);
    REQUIRE(!obake::is_exponentiable_v<void, void>);
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::is_exponentiable_v<float, __int128_t>);
    REQUIRE(obake::is_exponentiable_v<float, __uint128_t>);
    REQUIRE(obake::is_exponentiable_v<__int128_t &&, const double>);
    REQUIRE(obake::is_exponentiable_v<__uint128_t &&, const double>);
    REQUIRE(!obake::is_exponentiable_v<__uint128_t &&, __int128_t>);
    REQUIRE(!obake::is_exponentiable_v<__int128_t &&, __uint128_t>);
#endif
#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::Exponentiable<float, int>);
    REQUIRE(obake::Exponentiable<const double &, float &&>);
    REQUIRE(obake::Exponentiable<long double &, float &&>);
    REQUIRE(!obake::Exponentiable<int, const bool &>);
    REQUIRE(!obake::Exponentiable<short, unsigned char &&>);
    REQUIRE(!obake::Exponentiable<double, void>);
    REQUIRE(!obake::Exponentiable<void, double>);
    REQUIRE(!obake::Exponentiable<void, void>);
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::Exponentiable<float, __int128_t>);
    REQUIRE(obake::Exponentiable<__uint128_t, long double &>);
    REQUIRE(!obake::Exponentiable<int, const __int128_t>);
    REQUIRE(!obake::Exponentiable<__uint128_t, const __int128_t>);
#endif
#endif

    // Simple checks, and check the return types.
    REQUIRE(obake::pow(3., 5) == std::pow(3., 5));
    REQUIRE(obake::pow(5, 3.) == std::pow(5, 3.));
    REQUIRE(obake::pow(3., -2.) == std::pow(3., -2.));
    REQUIRE(obake::pow(3.f, -2.f) == std::pow(3.f, -2.f));
    REQUIRE(obake::pow(3.l, -2.l) == std::pow(3.l, -2.l));
    REQUIRE(std::is_same_v<decltype(obake::pow(3., 5)), double>);
    REQUIRE(std::is_same_v<decltype(obake::pow(5, 3.)), double>);
    REQUIRE(std::is_same_v<decltype(obake::pow(3.f, 5)), float>);
    REQUIRE(std::is_same_v<decltype(obake::pow(5, 3.f)), float>);
    REQUIRE(std::is_same_v<decltype(obake::pow(3.l, 5)), long double>);
    REQUIRE(std::is_same_v<decltype(obake::pow(5, 3.l)), long double>);
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::pow(3., __int128_t(5)) == std::pow(3., 5));
    REQUIRE(obake::pow(__uint128_t(5), 3.) == std::pow(5, 3.));
    REQUIRE(std::is_same_v<decltype(obake::pow(3., __int128_t(5))), double>);
    REQUIRE(std::is_same_v<decltype(obake::pow(3., __uint128_t(5))), double>);
    REQUIRE(std::is_same_v<decltype(obake::pow(__int128_t(5), 3.)), double>);
    REQUIRE(std::is_same_v<decltype(obake::pow(__uint128_t(5), 3.)), double>);
    REQUIRE(std::is_same_v<decltype(obake::pow(3.f, __int128_t(5))), float>);
    REQUIRE(std::is_same_v<decltype(obake::pow(3.f, __uint128_t(5))), float>);
    REQUIRE(std::is_same_v<decltype(obake::pow(__int128_t(5), 3.f)), float>);
    REQUIRE(std::is_same_v<decltype(obake::pow(__uint128_t(5), 3.f)), float>);
    REQUIRE(std::is_same_v<decltype(obake::pow(3.l, __int128_t(5))), long double>);
    REQUIRE(std::is_same_v<decltype(obake::pow(3.l, __uint128_t(5))), long double>);
    REQUIRE(std::is_same_v<decltype(obake::pow(__int128_t(5), 3.l)), long double>);
    REQUIRE(std::is_same_v<decltype(obake::pow(__uint128_t(5), 3.l)), long double>);
#endif

    // Check perfect forwarding.
#if !defined(OBAKE_COMPILER_IS_GCC) || __GNUC__ >= 8
    REQUIRE(noexcept(obake::pow(1.5, 1.5)));
    REQUIRE(noexcept(obake::pow(1.5, 1)));
    REQUIRE(noexcept(obake::pow(1, 1.5)));
    REQUIRE(noexcept(obake::pow(1.5f, 1.5f)));
    REQUIRE(noexcept(obake::pow(1.5f, 1)));
    REQUIRE(noexcept(obake::pow(1, 1.5f)));
#endif
}

TEST_CASE("pow_mp++_int")
{
    using int_t = mppp::integer<1>;

    // A few simple checks.
    REQUIRE(obake::pow(int_t{3}, 5) == 243);
    REQUIRE(obake::pow(3, int_t{5}) == 243);
    REQUIRE(obake::pow(3., int_t{5}) == std::pow(3., 5.));
    REQUIRE(obake::pow(int_t{5}, 3.) == std::pow(5., 3.));
    REQUIRE(!noexcept(obake::pow(int_t{5}, 3.)));
#if defined(MPPP_WITH_MPFR)
    REQUIRE(obake::pow(3.l, int_t{5}) == std::pow(3.l, 5.l));
    REQUIRE(obake::pow(int_t{5}, 3.l) == std::pow(5.l, 3.l));
#else
    REQUIRE(!obake::is_exponentiable_v<long double, int_t>);
    REQUIRE(!obake::is_exponentiable_v<int_t, long double>);
#endif
    REQUIRE(!obake::is_exponentiable_v<int_t, std::string>);
}

TEST_CASE("pow_mp++_rat")
{
    using rat_t = mppp::rational<1>;

    // A few simple checks.
    REQUIRE(obake::pow(rat_t{3, 2}, 5) == rat_t{243, 32});
    REQUIRE(obake::pow(3., rat_t{5, 2}) == std::pow(3., 5. / 2.));
    REQUIRE(obake::pow(rat_t{5, 2}, 3.) == std::pow(5. / 2., 3.));
    REQUIRE(!noexcept(obake::pow(rat_t{5}, 3.)));
#if defined(MPPP_WITH_MPFR)
    REQUIRE(obake::pow(3.l, rat_t{5}) == std::pow(3.l, 5.l));
    REQUIRE(obake::pow(rat_t{5}, 3.l) == std::pow(5.l, 3.l));
#else
    REQUIRE(!obake::is_exponentiable_v<long double, rat_t>);
    REQUIRE(!obake::is_exponentiable_v<rat_t, long double>);
#endif
    REQUIRE(!obake::is_exponentiable_v<rat_t, std::string>);
}

#if defined(MPPP_WITH_MPFR)

TEST_CASE("pow_mp++_real")
{
    using mppp::real;

    // A few simple checks.
    REQUIRE(obake::pow(real{3}, 5) == 243);
    REQUIRE(obake::pow(3, real{5}) == 243);
    REQUIRE(obake::pow(3., real{5}) == std::pow(3., 5.));
    REQUIRE(obake::pow(real{5}, 3.) == std::pow(5., 3.));
    REQUIRE(!noexcept(obake::pow(real{5}, 3.)));
    REQUIRE(!obake::is_exponentiable_v<real, std::string>);
}

#endif

#if defined(MPPP_WITH_QUADMATH)

TEST_CASE("pow_mp++_real128")
{
    using mppp::real128;

    // A few simple checks.
    REQUIRE(obake::pow(real128{3}, 5) == 243);
    REQUIRE(obake::pow(3, real128{5}) == 243);
    REQUIRE(obake::pow(3., real128{5}) == std::pow(3., 5.));
    REQUIRE(obake::pow(real128{5}, 3.) == std::pow(5., 3.));
    REQUIRE(!noexcept(obake::pow(real128{5}, 3.)));
    REQUIRE(!obake::is_exponentiable_v<real128, std::string>);
}

#endif

// Test the customisation machinery.

// A new type.
struct foo0 {
};

// A non-constexpr function.
inline void non_constexpr() {}

// Customise obake::pow() for foo0.
namespace obake::customisation
{

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, foo0> &&SameCvr<U, foo0> inline constexpr auto pow<T, U>
#else
inline constexpr auto pow<T, U, std::enable_if_t<is_same_cvr_v<T, foo0> && is_same_cvr_v<U, foo0>>>
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

} // namespace obake::customisation

template <int>
struct bar {
};

TEST_CASE("pow_custom")
{
    REQUIRE(!obake::is_exponentiable_v<foo0, int>);
    REQUIRE(!obake::is_exponentiable_v<int, foo0>);
    REQUIRE(obake::is_exponentiable_v<foo0, foo0>);

    REQUIRE(obake::pow(foo0{}, foo0{}) == 1);
    foo0 f;
    REQUIRE(obake::pow(f, foo0{}) == 2);
    REQUIRE(obake::pow(foo0{}, f) == 2);
    REQUIRE(obake::pow(std::move(f), foo0{}) == 1);
    REQUIRE(obake::pow(foo0{}, std::move(f)) == 1);

    // Check that constexpr is preserved if only rvalues are involved.
    [[maybe_unused]] bar<obake::pow(foo0{}, foo0{})> b0;
    // This will result in a compilation error.
    // [[maybe_unused]] bar<obake::pow(foo0{}, f)> b1;
}
