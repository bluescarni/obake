// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <obake/config.hpp>
#include <obake/polynomials/monomial_mul.hpp>
#include <obake/symbols.hpp>

#include "catch.hpp"

using namespace obake;

namespace ns
{

// ADL-based customisation.
struct mm00 {
};

void monomial_mul(mm00 &, const mm00 &, const mm00 &, const symbol_set &);

struct nomm00 {
};

// Wrong prototype.
void monomial_mul(nomm00 &, const nomm00 &, const nomm00 &);

} // namespace ns

struct mm01 {
};

struct nomm01 {
};

// External customisation.
namespace obake::customisation
{

template <>
inline constexpr auto monomial_mul<mm01 &, const mm01 &, const mm01 &> = [](auto &, const auto &, const auto &,
                                                                            const symbol_set &) constexpr noexcept {};

template <>
inline constexpr auto monomial_mul<nomm01 &, const nomm01 &, const nomm01 &> = [](auto &, const auto &,
                                                                                  const auto &) constexpr noexcept {};

} // namespace obake::customisation

TEST_CASE("monomial_mul_test")
{
    REQUIRE(!is_multipliable_monomial_v<void, void, void>);

    REQUIRE(!is_multipliable_monomial_v<ns::mm00 &, void, void>);
    REQUIRE(!is_multipliable_monomial_v<void, ns::mm00 &, void>);
    REQUIRE(!is_multipliable_monomial_v<void, void, ns::mm00 &>);

    REQUIRE(is_multipliable_monomial_v<ns::mm00 &, const ns::mm00 &, const ns::mm00 &>);
    REQUIRE(!is_multipliable_monomial_v<const ns::mm00 &, const ns::mm00 &, const ns::mm00 &>);
    REQUIRE(!is_multipliable_monomial_v<ns::mm00 &&, const ns::mm00 &, const ns::mm00 &>);

    REQUIRE(!is_multipliable_monomial_v<ns::nomm00 &, const ns::nomm00 &, const ns::nomm00 &>);
    REQUIRE(!is_multipliable_monomial_v<const ns::nomm00 &, const ns::nomm00 &, const ns::nomm00 &>);
    REQUIRE(!is_multipliable_monomial_v<ns::nomm00 &&, const ns::nomm00 &, const ns::nomm00 &>);

    REQUIRE(is_multipliable_monomial_v<mm01 &, const mm01 &, const mm01 &>);
    REQUIRE(!is_multipliable_monomial_v<const mm01 &, const mm01 &, const mm01 &>);
    REQUIRE(!is_multipliable_monomial_v<mm01 &&, const mm01 &, const mm01 &>);

    REQUIRE(!is_multipliable_monomial_v<nomm01 &, const nomm01 &, const nomm01 &>);
    REQUIRE(!is_multipliable_monomial_v<const nomm01 &, const nomm01 &, const nomm01 &>);
    REQUIRE(!is_multipliable_monomial_v<nomm01 &&, const nomm01 &, const nomm01 &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!MultipliableMonomial<void, void, void>);

    REQUIRE(!MultipliableMonomial<ns::mm00 &, void, void>);
    REQUIRE(!MultipliableMonomial<void, ns::mm00 &, void>);
    REQUIRE(!MultipliableMonomial<void, void, ns::mm00 &>);

    REQUIRE(MultipliableMonomial<ns::mm00 &, const ns::mm00 &, const ns::mm00 &>);
    REQUIRE(!MultipliableMonomial<const ns::mm00 &, const ns::mm00 &, const ns::mm00 &>);
    REQUIRE(!MultipliableMonomial<ns::mm00 &&, const ns::mm00 &, const ns::mm00 &>);

    REQUIRE(!MultipliableMonomial<ns::nomm00 &, const ns::nomm00 &, const ns::nomm00 &>);
    REQUIRE(!MultipliableMonomial<const ns::nomm00 &, const ns::nomm00 &, const ns::nomm00 &>);
    REQUIRE(!MultipliableMonomial<ns::nomm00 &&, const ns::nomm00 &, const ns::nomm00 &>);

    REQUIRE(MultipliableMonomial<mm01 &, const mm01 &, const mm01 &>);
    REQUIRE(!MultipliableMonomial<const mm01 &, const mm01 &, const mm01 &>);
    REQUIRE(!MultipliableMonomial<mm01 &&, const mm01 &, const mm01 &>);

    REQUIRE(!MultipliableMonomial<nomm01 &, const nomm01 &, const nomm01 &>);
    REQUIRE(!MultipliableMonomial<const nomm01 &, const nomm01 &, const nomm01 &>);
    REQUIRE(!MultipliableMonomial<nomm01 &&, const nomm01 &, const nomm01 &>);
#endif
}
