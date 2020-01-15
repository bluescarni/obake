// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <obake/config.hpp>
#include <obake/polynomials/monomial_pow.hpp>
#include <obake/symbols.hpp>

#include "catch.hpp"

using namespace obake;

namespace ns
{

// ADL-based customisation.
struct mp00 {
};

mp00 monomial_pow(const mp00 &, int, const symbol_set &);

struct mp00a {
};

// Disable const overload.
mp00a monomial_pow(mp00a &, int, const symbol_set &);

struct nomp00 {
};

// Wrong prototype.
void monomial_pow(const nomp00 &, int, const symbol_set &);

} // namespace ns

struct mp01 {
};

struct nomp01 {
};

// External customisation.
namespace obake::customisation
{

template <>
inline constexpr auto monomial_pow<const mp01 &, int &&> = [](const auto &, int, const symbol_set &) constexpr noexcept
{
    return mp01{};
};

template <>
inline constexpr auto monomial_pow<const nomp01 &, int &&> = [](const auto &, int,
                                                                const symbol_set &) constexpr noexcept {};

} // namespace obake::customisation

TEST_CASE("monomial_pow_test")
{
    REQUIRE(!is_exponentiable_monomial_v<void, void>);

    REQUIRE(!is_exponentiable_monomial_v<ns::mp00 &, void>);
    REQUIRE(!is_exponentiable_monomial_v<void, ns::mp00 &>);

    REQUIRE(is_exponentiable_monomial_v<ns::mp00 &, int &>);
    REQUIRE(is_exponentiable_monomial_v<const ns::mp00 &, int>);
    REQUIRE(is_exponentiable_monomial_v<const ns::mp00 &, const int>);

    REQUIRE(is_exponentiable_monomial_v<ns::mp00a &, int &>);
    REQUIRE(!is_exponentiable_monomial_v<const ns::mp00a &, int>);
    REQUIRE(!is_exponentiable_monomial_v<const ns::mp00a &, const int>);

    REQUIRE(!is_exponentiable_monomial_v<ns::nomp00 &, int>);
    REQUIRE(!is_exponentiable_monomial_v<const ns::nomp00 &, const int &>);
    REQUIRE(!is_exponentiable_monomial_v<ns::nomp00 &&, int>);

    REQUIRE(!is_exponentiable_monomial_v<mp01 &, int>);
    REQUIRE(is_exponentiable_monomial_v<const mp01 &, int>);
    REQUIRE(!is_exponentiable_monomial_v<mp01 &&, int>);

    REQUIRE(!is_exponentiable_monomial_v<nomp01 &, int>);
    REQUIRE(!is_exponentiable_monomial_v<const nomp01 &, int>);
    REQUIRE(!is_exponentiable_monomial_v<nomp01 &&, int>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!ExponentiableMonomial<void, void>);

    REQUIRE(!ExponentiableMonomial<ns::mp00 &, void>);
    REQUIRE(!ExponentiableMonomial<void, ns::mp00 &>);

    REQUIRE(ExponentiableMonomial<ns::mp00 &, int &>);
    REQUIRE(ExponentiableMonomial<const ns::mp00 &, int>);
    REQUIRE(ExponentiableMonomial<const ns::mp00 &, const int>);

    REQUIRE(ExponentiableMonomial<ns::mp00a &, int &>);
    REQUIRE(!ExponentiableMonomial<const ns::mp00a &, int>);
    REQUIRE(!ExponentiableMonomial<const ns::mp00a &, const int>);

    REQUIRE(!ExponentiableMonomial<ns::nomp00 &, int>);
    REQUIRE(!ExponentiableMonomial<const ns::nomp00 &, const int &>);
    REQUIRE(!ExponentiableMonomial<ns::nomp00 &&, int>);

    REQUIRE(!ExponentiableMonomial<mp01 &, int>);
    REQUIRE(ExponentiableMonomial<const mp01 &, int>);
    REQUIRE(!ExponentiableMonomial<mp01 &&, int>);

    REQUIRE(!ExponentiableMonomial<nomp01 &, int>);
    REQUIRE(!ExponentiableMonomial<const nomp01 &, int>);
    REQUIRE(!ExponentiableMonomial<nomp01 &&, int>);
#endif
}
