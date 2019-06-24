// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/polynomials/monomial_mul.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <piranha/config.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

namespace ns
{

// ADL-based customisation.
struct mm00 {
};

void monomial_mul(mm00 &, const mm00 &, const mm00 &, const symbol_set &);

} // namespace ns

struct mm01 {
};

namespace piranha::customisation
{

template <typename T>
inline constexpr auto monomial_mul<T &, const T &, const T &> = [](auto &, const auto &, const auto &,
                                                                   const symbol_set &) constexpr noexcept {};

} // namespace piranha::customisation

TEST_CASE("monomial_mul_test")
{
    REQUIRE(!is_multipliable_monomial_v<void, void, void>);

    REQUIRE(!is_multipliable_monomial_v<ns::mm00 &, void, void>);
    REQUIRE(!is_multipliable_monomial_v<void, ns::mm00 &, void>);
    REQUIRE(!is_multipliable_monomial_v<void, void, ns::mm00 &>);

    REQUIRE(is_multipliable_monomial_v<ns::mm00 &, const ns::mm00 &, const ns::mm00 &>);
    REQUIRE(!is_multipliable_monomial_v<const ns::mm00 &, const ns::mm00 &, const ns::mm00 &>);
    REQUIRE(!is_multipliable_monomial_v<ns::mm00 &&, const ns::mm00 &, const ns::mm00 &>);

    REQUIRE(is_multipliable_monomial_v<mm01 &, const mm01 &, const mm01 &>);
    REQUIRE(!is_multipliable_monomial_v<const mm01 &, const mm01 &, const mm01 &>);
    REQUIRE(!is_multipliable_monomial_v<mm01 &&, const mm01 &, const mm01 &>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!MultipliableMonomial<void, void, void>);

    REQUIRE(!MultipliableMonomial<ns::mm00 &, void, void>);
    REQUIRE(!MultipliableMonomial<void, ns::mm00 &, void>);
    REQUIRE(!MultipliableMonomial<void, void, ns::mm00 &>);

    REQUIRE(MultipliableMonomial<ns::mm00 &, const ns::mm00 &, const ns::mm00 &>);
    REQUIRE(!MultipliableMonomial<const ns::mm00 &, const ns::mm00 &, const ns::mm00 &>);
    REQUIRE(!MultipliableMonomial<ns::mm00 &&, const ns::mm00 &, const ns::mm00 &>);

    REQUIRE(MultipliableMonomial<mm01 &, const mm01 &, const mm01 &>);
    REQUIRE(!MultipliableMonomial<const mm01 &, const mm01 &, const mm01 &>);
    REQUIRE(!MultipliableMonomial<mm01 &&, const mm01 &, const mm01 &>);
#endif
}
