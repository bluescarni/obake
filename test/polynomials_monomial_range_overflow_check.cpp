// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/polynomials/monomial_range_overflow_check.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <list>
#include <vector>

#include <piranha/config.hpp>
#include <piranha/symbols.hpp>

using namespace piranha;

namespace ns
{

// ADL-based customisation.
struct mroc00 {
};

bool monomial_range_overflow_check(const std::vector<mroc00> &, const std::vector<mroc00> &, const symbol_set &)
{
    return false;
}

struct nomroc00 {
};

// Wrong prototype.
void monomial_range_overflow_check(const std::vector<nomroc00> &, const std::vector<nomroc00> &, const symbol_set &);

} // namespace ns

struct mroc01 {
};

struct nomroc01 {
};

// External customisation.
namespace piranha::customisation
{

template <>
inline constexpr auto monomial_range_overflow_check<const std::vector<mroc01> &, const std::vector<mroc01> &> = [
](const auto &, const auto &, const symbol_set &) constexpr noexcept
{
    return false;
};

template <>
inline constexpr auto monomial_range_overflow_check<const std::vector<nomroc01> &, const std::vector<nomroc01> &> = [
](const auto &, const auto &, const symbol_set &) constexpr noexcept {};

} // namespace piranha::customisation

TEST_CASE("monomial_range_overflow_check_test")
{
    REQUIRE(!is_overflow_testable_monomial_range_v<void, void>);

    REQUIRE(!is_overflow_testable_monomial_range_v<std::vector<ns::mroc00>, void>);
    REQUIRE(!is_overflow_testable_monomial_range_v<void, std::vector<ns::mroc00>>);

    REQUIRE(!monomial_range_overflow_check(std::vector<ns::mroc00>{}, std::vector<ns::mroc00>{}, symbol_set{}));
    REQUIRE(monomial_range_overflow_check(std::vector<ns::mroc00>{}, std::list<ns::mroc00>{}, symbol_set{}));
    REQUIRE(monomial_range_overflow_check(std::list<ns::mroc00>{}, std::vector<ns::mroc00>{}, symbol_set{}));

    REQUIRE(!is_overflow_testable_monomial_range_v<std::vector<ns::nomroc00>, std::vector<ns::nomroc00>>);

    REQUIRE(!is_overflow_testable_monomial_range_v<std::vector<mroc01>, void>);
    REQUIRE(!is_overflow_testable_monomial_range_v<void, std::vector<mroc01>>);

    REQUIRE(monomial_range_overflow_check(std::vector<mroc01>{}, std::vector<mroc01>{}, symbol_set{}));
    REQUIRE(!monomial_range_overflow_check(static_cast<const std::vector<mroc01> &>(std::vector<mroc01>{}),
                                           static_cast<const std::vector<mroc01> &>(std::vector<mroc01>{}),
                                           symbol_set{}));

    REQUIRE(is_overflow_testable_monomial_range_v<std::vector<nomroc01>, std::vector<nomroc01>>);
    REQUIRE(!is_overflow_testable_monomial_range_v<const std::vector<nomroc01> &, const std::vector<nomroc01> &>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!OverflowTestableMonomialRange<void, void>);

    REQUIRE(!OverflowTestableMonomialRange<std::vector<ns::mroc00>, void>);
    REQUIRE(!OverflowTestableMonomialRange<void, std::vector<ns::mroc00>>);

    REQUIRE(!OverflowTestableMonomialRange<std::vector<ns::nomroc00>, std::vector<ns::nomroc00>>);

    REQUIRE(!OverflowTestableMonomialRange<std::vector<mroc01>, void>);
    REQUIRE(!OverflowTestableMonomialRange<void, std::vector<mroc01>>);

    REQUIRE(OverflowTestableMonomialRange<std::vector<nomroc01>, std::vector<nomroc01>>);
    REQUIRE(!OverflowTestableMonomialRange<const std::vector<nomroc01> &, const std::vector<nomroc01> &>);
#endif
}
