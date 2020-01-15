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

#include <obake/config.hpp>
#include <obake/math/negate.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("negate_arith")
{
    REQUIRE(!is_negatable_v<void>);
    REQUIRE(is_negatable_v<float>);
    REQUIRE(is_negatable_v<int>);
    REQUIRE(is_negatable_v<double &&>);
    REQUIRE(!is_negatable_v<const long double>);
    REQUIRE(is_negatable_v<int &>);
    REQUIRE(!is_negatable_v<const int &>);
    REQUIRE(!is_negatable_v<const char &>);

#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(is_negatable_v<__int128_t>);
    REQUIRE(is_negatable_v<__uint128_t>);
    REQUIRE(is_negatable_v<__int128_t &&>);
    REQUIRE(!is_negatable_v<const __uint128_t>);
    REQUIRE(!is_negatable_v<const __uint128_t &>);
    REQUIRE(is_negatable_v<__int128_t &>);
#endif

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Negatable<void>);
    REQUIRE(Negatable<float>);
    REQUIRE(Negatable<int>);
    REQUIRE(Negatable<double &&>);
    REQUIRE(!Negatable<const long double>);
    REQUIRE(Negatable<int &>);
    REQUIRE(!Negatable<const int &>);
    REQUIRE(!Negatable<const char &>);

#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(Negatable<__int128_t>);
    REQUIRE(Negatable<__uint128_t>);
    REQUIRE(Negatable<__int128_t &&>);
    REQUIRE(!Negatable<const __uint128_t>);
    REQUIRE(!Negatable<const __uint128_t &>);
    REQUIRE(Negatable<__int128_t &>);
#endif
#endif

    REQUIRE(std::is_same_v<decltype(negate(4)), int &&>);
    REQUIRE(negate(4) == -4);

    int n = 4;
    REQUIRE(&negate(n) == &n);
    REQUIRE(std::is_same_v<decltype(negate(n)), int &>);
    REQUIRE(n == -4);
    REQUIRE(&negate(n) == &n);
    REQUIRE(n == 4);

    unsigned u = 4;
    REQUIRE(&negate(u) == &u);
    REQUIRE(u == std::numeric_limits<unsigned>::max() - 3u);
    REQUIRE(&negate(u) == &u);
    REQUIRE(u == 4u);

    auto x = -6.;
    REQUIRE(&negate(x) == &x);
    REQUIRE(x == 6.);
    REQUIRE(&negate(x) == &x);
    REQUIRE(x == -6.);

#if defined(OBAKE_HAVE_GCC_INT128)

    auto nn = __int128_t(5);
    REQUIRE(&negate(nn) == &nn);
    REQUIRE(nn == -5);
    REQUIRE(&negate(nn) == &nn);
    REQUIRE(nn == 5);

#endif
}

TEST_CASE("negate_mppp")
{
    using int_t = mppp::integer<1>;
    using rat_t = mppp::rational<1>;

    REQUIRE(is_negatable_v<int_t>);
    REQUIRE(is_negatable_v<int_t &>);
    REQUIRE(is_negatable_v<int_t &&>);
    REQUIRE(!is_negatable_v<const int_t &&>);
    REQUIRE(!is_negatable_v<const int_t>);
    REQUIRE(!is_negatable_v<const int_t &>);

    REQUIRE(is_negatable_v<rat_t>);
    REQUIRE(is_negatable_v<rat_t &>);
    REQUIRE(is_negatable_v<rat_t &&>);
    REQUIRE(!is_negatable_v<const rat_t &&>);
    REQUIRE(!is_negatable_v<const rat_t>);
    REQUIRE(!is_negatable_v<const rat_t &>);

    REQUIRE(negate(int_t{123}) == -123);
    int_t n{-456};
    REQUIRE(&negate(n) == &n);
    REQUIRE(n == 456);

    REQUIRE(negate(rat_t{123, 45}) == -rat_t{123} / 45);
    rat_t q{-456, 7};
    REQUIRE(&negate(q) == &q);
    REQUIRE(q == 456 / rat_t{7});

#if defined(MPPP_WITH_MPFR)
    REQUIRE(is_negatable_v<mppp::real>);
    REQUIRE(is_negatable_v<mppp::real &>);
    REQUIRE(is_negatable_v<mppp::real &&>);
    REQUIRE(!is_negatable_v<const mppp::real &&>);
    REQUIRE(!is_negatable_v<const mppp::real>);
    REQUIRE(!is_negatable_v<const mppp::real &>);

    REQUIRE(negate(mppp::real{123}) == -mppp::real{123});
    mppp::real r{-456};
    REQUIRE(&negate(r) == &r);
    REQUIRE(r == 456);
#endif
}

struct noadl00 {
};

struct noadl01 {
};

// Wrong prototype.
void negate(noadl01 &, int);

// ADL implementations.
struct adl00 {
};

void negate(adl00 &);

struct adl01 {
};

void negate(adl01 &);
void negate(adl01 &&);

// External customisation point.
struct ext00 {
};

// Internal customisation point.
struct int00 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext00> inline constexpr auto negate<T>
#else
inline constexpr auto negate<T, std::enable_if_t<is_same_cvr_v<T, ext00>>>
#endif
    = [](auto &&) constexpr noexcept {};

namespace internal
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, int00> inline constexpr auto negate<T>
#else
inline constexpr auto negate<T, std::enable_if_t<is_same_cvr_v<T, int00>>>
#endif
    = [](auto &&) constexpr noexcept {};

} // namespace internal

} // namespace obake::customisation

TEST_CASE("negate_customisation")
{
    REQUIRE(!is_negatable_v<noadl00>);
    REQUIRE(!is_negatable_v<noadl00 &>);
    REQUIRE(!is_negatable_v<noadl00 &&>);
    REQUIRE(!is_negatable_v<const noadl00 &>);

    REQUIRE(!is_negatable_v<noadl01>);
    REQUIRE(!is_negatable_v<noadl01 &>);
    REQUIRE(!is_negatable_v<noadl01 &&>);
    REQUIRE(!is_negatable_v<const noadl01 &>);

    REQUIRE(!is_negatable_v<adl00>);
    REQUIRE(is_negatable_v<adl00 &>);
    REQUIRE(!is_negatable_v<adl00 &&>);
    REQUIRE(!is_negatable_v<const adl00 &>);

    REQUIRE(is_negatable_v<adl01>);
    REQUIRE(is_negatable_v<adl01 &>);
    REQUIRE(is_negatable_v<adl01 &&>);
    REQUIRE(!is_negatable_v<const adl01 &&>);
    REQUIRE(!is_negatable_v<const adl01 &>);

    REQUIRE(is_negatable_v<ext00>);
    REQUIRE(is_negatable_v<ext00 &>);
    REQUIRE(is_negatable_v<ext00 &&>);
    REQUIRE(is_negatable_v<const ext00 &&>);
    REQUIRE(is_negatable_v<const ext00 &>);

    REQUIRE(is_negatable_v<int00>);
    REQUIRE(is_negatable_v<int00 &>);
    REQUIRE(is_negatable_v<int00 &&>);
    REQUIRE(is_negatable_v<const int00 &&>);
    REQUIRE(is_negatable_v<const int00 &>);

#if defined(OBAKE_HAVE_CONCEPTS)

    REQUIRE(!Negatable<noadl00>);
    REQUIRE(!Negatable<noadl00 &>);
    REQUIRE(!Negatable<noadl00 &&>);
    REQUIRE(!Negatable<const noadl00 &>);

    REQUIRE(!Negatable<noadl01>);
    REQUIRE(!Negatable<noadl01 &>);
    REQUIRE(!Negatable<noadl01 &&>);
    REQUIRE(!Negatable<const noadl01 &>);

    REQUIRE(!Negatable<adl00>);
    REQUIRE(Negatable<adl00 &>);
    REQUIRE(!Negatable<adl00 &&>);
    REQUIRE(!Negatable<const adl00 &>);

    REQUIRE(Negatable<adl01>);
    REQUIRE(Negatable<adl01 &>);
    REQUIRE(Negatable<adl01 &&>);
    REQUIRE(!Negatable<const adl01 &&>);
    REQUIRE(!Negatable<const adl01 &>);

    REQUIRE(Negatable<ext00>);
    REQUIRE(Negatable<ext00 &>);
    REQUIRE(Negatable<ext00 &&>);
    REQUIRE(Negatable<const ext00 &&>);
    REQUIRE(Negatable<const ext00 &>);

    REQUIRE(Negatable<int00>);
    REQUIRE(Negatable<int00 &>);
    REQUIRE(Negatable<int00 &&>);
    REQUIRE(Negatable<const int00 &&>);
    REQUIRE(Negatable<const int00 &>);

#endif
}