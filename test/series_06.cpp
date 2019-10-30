// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <limits>
#include <type_traits>
#include <utility>

#include <mp++/exceptions.hpp>
#include <mp++/rational.hpp>

#include <obake/config.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/series.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using rat_t = mppp::rational<1>;

using namespace obake;

struct foo {
};

namespace ns
{

using pm_t = packed_monomial<int>;

// ADL-based customization.
struct tag00 {
};

inline bool series_div(const series<pm_t, rat_t, tag00> &, const series<pm_t, rat_t, tag00> &)
{
    return true;
}

// External customisation.
struct tag01 {
};

using s1_t = series<pm_t, rat_t, tag01>;

} // namespace ns

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ns::s1_t> inline constexpr auto series_div<T, T>
#else
inline constexpr auto series_div<T, T, std::enable_if_t<is_same_cvr_v<T, ns::s1_t>>>
#endif
    = [](const auto &, const auto &) { return false; };

} // namespace obake::customisation

// Customisation points.
TEST_CASE("series_div_customisation")
{
    obake_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<int>;

    REQUIRE(series<pm_t, rat_t, ns::tag00>{} / series<pm_t, rat_t, ns::tag00>{});
    REQUIRE(!(ns::s1_t{} / ns::s1_t{}));

    REQUIRE(!is_divisible_v<series<pm_t, rat_t, ns::tag00>, void>);
    REQUIRE(!is_divisible_v<void, series<pm_t, rat_t, ns::tag00>>);
    REQUIRE(!is_divisible_v<ns::s1_t, void>);
    REQUIRE(!is_divisible_v<void, ns::s1_t>);
}

TEST_CASE("series_div")
{
    using pm_t = packed_monomial<int>;
    using s1_t = polynomial<pm_t, rat_t>;
    using s11_t = polynomial<pm_t, s1_t>;
    using s2_t = polynomial<pm_t, double>;

    REQUIRE(std::is_same_v<s1_t, decltype(s1_t{} / 3)>);
    REQUIRE((s1_t{} / 3).empty());
    REQUIRE(s1_t{1} / 3 == rat_t{1, 3});

    auto [x, y] = make_polynomials<s1_t>("x", "y");
    auto [z] = make_polynomials<s11_t>("z");

    REQUIRE(x / 3 == rat_t{1, 3} * x);
    REQUIRE((x / 3 - y / -5) * (x / 3 + y / -5) == rat_t{1, 9} * x * x - y * y * rat_t{1, 25});
    REQUIRE(((x * x + y * y) * z + 1) / 4 == ((x * x + y * y) * z + 1) * rat_t{1, 4});
    REQUIRE(std::is_same_v<polynomial<pm_t, s2_t>, decltype(((x * x + y * y) * z + 1) / 4.)>);

    OBAKE_REQUIRES_THROWS_CONTAINS(x / 0, mppp::zero_division_error, "");

    REQUIRE(std::is_same_v<s2_t, decltype(s1_t{} / 3.)>);
    REQUIRE((s2_t{} / 3.).empty());
    REQUIRE(s2_t{1} / 2. == 1. / 2.);

    if (std::numeric_limits<double>::has_infinity) {
        // Test cancellations via division by infinity.
        auto [a, b] = make_polynomials<s2_t>("a", "b");

        REQUIRE((((a + b) * (a - b)) / std::numeric_limits<double>::infinity()).empty());
    }

    // In-place testing.
    auto tmp(x);
    tmp /= 3;
    REQUIRE(tmp == x / 3);
    std::move(tmp) /= 3;
    REQUIRE(tmp == x / 9);

    // Test that unsupported operand combinations are disabled.
    REQUIRE(!is_detected_v<detail::div_t, s1_t, void>);
    REQUIRE(!is_detected_v<detail::div_t, void, s1_t>);
    REQUIRE(!is_detected_v<detail::div_t, s1_t, s1_t>);
    REQUIRE(!is_detected_v<detail::div_t, int, s1_t>);
    REQUIRE(!is_detected_v<detail::div_t, s2_t, s1_t>);
    REQUIRE(!is_detected_v<detail::div_t, s1_t, s2_t>);
    REQUIRE(!is_detected_v<detail::div_t, s11_t, s1_t>);
    REQUIRE(!is_detected_v<detail::div_t, s1_t, s11_t>);
    REQUIRE(!is_compound_divisible_v<s1_t &, const s1_t &>);
    REQUIRE(!is_compound_divisible_v<s1_t &, void>);
    REQUIRE(!is_compound_divisible_v<s11_t &, const s11_t &>);
    REQUIRE(!is_compound_divisible_v<s11_t &, const s1_t &>);
    REQUIRE(!is_compound_divisible_v<int &, const s1_t &>);
}
