// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <limits>
#include <type_traits>
#include <utility>

#include <mp++/exceptions.hpp>
#include <mp++/rational.hpp>

#include <piranha/byte_size.hpp>
#include <piranha/config.hpp>
#include <piranha/math/degree.hpp>
#include <piranha/math/p_degree.hpp>
#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/polynomials/polynomial.hpp>
#include <piranha/series.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using rat_t = mppp::rational<1>;

using namespace piranha;

TEST_CASE("series_byte_size")
{
    piranha_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    s1.set_symbol_set(symbol_set{"x", "y"});
    s1.add_term(pm_t{1, 1}, 1);
    s1.add_term(pm_t{2, 2}, 2);

    REQUIRE(byte_size(s1) > sizeof(s1_t));

    // s2 has more terms than s1.
    s1_t s2;
    s2.set_symbol_set(symbol_set{"x", "y"});
    s2.add_term(pm_t{1, 1}, 1);
    s2.add_term(pm_t{2, 2}, 2);
    s2.add_term(pm_t{3, 3}, 3);

    REQUIRE(byte_size(s2) >= byte_size(s1));

    // s3 has more symbols than s2.
    s1_t s3;
    s3.set_symbol_set(symbol_set{"x", "y", "z"});
    s3.add_term(pm_t{1, 1, 1}, 1);
    s3.add_term(pm_t{2, 2, 2}, 2);
    s3.add_term(pm_t{3, 3, 3}, 3);

    REQUIRE(byte_size(s3) >= byte_size(s2));
}

TEST_CASE("series_degree")
{
    using pm_t = packed_monomial<int>;
    using s1_t = polynomial<pm_t, rat_t>;
    using s11_t = polynomial<pm_t, s1_t>;

    REQUIRE(is_with_degree_v<s1_t>);
    REQUIRE(is_with_degree_v<const s1_t>);
    REQUIRE(is_with_degree_v<s1_t &>);
    REQUIRE(is_with_degree_v<const s1_t &>);
    REQUIRE(is_with_degree_v<s1_t &&>);

    REQUIRE(is_with_degree_v<s11_t>);
    REQUIRE(is_with_degree_v<const s11_t>);
    REQUIRE(is_with_degree_v<s11_t &>);
    REQUIRE(is_with_degree_v<const s11_t &>);
    REQUIRE(is_with_degree_v<s11_t &&>);

    {
        REQUIRE(degree(s1_t{}) == 0);

        auto [x, y, z] = make_polynomials<s1_t>("x", "y", "z");
        REQUIRE(std::is_same_v<decltype(degree(x)), int>);
        REQUIRE(degree(x * 0 + 1) == 0);
        REQUIRE(degree(x) == 1);
        REQUIRE(degree(y) == 1);
        REQUIRE(degree(z) == 1);

        REQUIRE(degree(x * x) == 2);
        REQUIRE(degree(y * x) == 2);
        REQUIRE(degree(z * z) == 2);
        REQUIRE(degree((x + y) * (x - y)) == 2);
        REQUIRE(degree((x + y) * (x - y) - z) == 2);
        REQUIRE(degree((x + y) * (x - y) - x * z * y) == 3);
        REQUIRE(degree((x + y) * (x - y) - x * z * y + 1) == 3);
    }

    {
        REQUIRE(degree(s11_t{}) == 0);

        auto [y, z] = make_polynomials<s11_t>("y", "z");
        auto [x] = make_polynomials<s1_t>("x");
        REQUIRE(std::is_same_v<decltype(degree(x)), int>);
        REQUIRE(std::is_same_v<decltype(degree(y)), int>);
        REQUIRE(std::is_same_v<decltype(degree(x * y)), int>);
        REQUIRE(degree(x * 0 + 1) == 0);
        REQUIRE(degree(x) == 1);
        REQUIRE(degree(y * 0 + 1) == 0);
        REQUIRE(degree(y) == 1);
        REQUIRE(degree(z) == 1);

        REQUIRE(degree(x * x) == 2);
        REQUIRE(degree(y * x) == 2);
        REQUIRE(degree(z * z) == 2);
        REQUIRE(degree((x + y) * (x - y)) == 2);
        REQUIRE(degree((x + y) * (x - y) - z) == 2);
        REQUIRE(degree((x + y) * (x - y) - x * z * y) == 3);
        REQUIRE(degree((x + y) * (x - y) - x * z * y + 1) == 3);
    }
}

TEST_CASE("series_p_degree")
{
    using pm_t = packed_monomial<int>;
    using s1_t = polynomial<pm_t, rat_t>;
    using s11_t = polynomial<pm_t, s1_t>;

    REQUIRE(is_with_p_degree_v<s1_t>);
    REQUIRE(is_with_p_degree_v<const s1_t>);
    REQUIRE(is_with_p_degree_v<s1_t &>);
    REQUIRE(is_with_p_degree_v<const s1_t &>);
    REQUIRE(is_with_p_degree_v<s1_t &&>);

    REQUIRE(is_with_p_degree_v<s11_t>);
    REQUIRE(is_with_p_degree_v<const s11_t>);
    REQUIRE(is_with_p_degree_v<s11_t &>);
    REQUIRE(is_with_p_degree_v<const s11_t &>);
    REQUIRE(is_with_p_degree_v<s11_t &&>);

    {
        REQUIRE(p_degree(s1_t{}, symbol_set{}) == 0);
        REQUIRE(p_degree(s1_t{}, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(s1_t{}, symbol_set{"x", "y", "z"}) == 0);

        auto [x, y, z] = make_polynomials<s1_t>("x", "y", "z");
        REQUIRE(std::is_same_v<decltype(p_degree(x, symbol_set{})), int>);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"x", "y", "z"}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"x", "z"}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"y", "z"}) == 0);
        REQUIRE(p_degree(x, symbol_set{}) == 0);
        REQUIRE(p_degree(x, symbol_set{"x"}) == 1);
        REQUIRE(p_degree(x, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(x, symbol_set{"z"}) == 0);
        REQUIRE(p_degree(x, symbol_set{"x", "y"}) == 1);
        REQUIRE(p_degree(x, symbol_set{"x", "z"}) == 1);
        REQUIRE(p_degree(x, symbol_set{"y", "z"}) == 0);
        REQUIRE(p_degree(y, symbol_set{"y"}) == 1);
        REQUIRE(p_degree(y, symbol_set{}) == 0);
        REQUIRE(p_degree(y, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(y, symbol_set{"x", "y"}) == 1);
        REQUIRE(p_degree(y, symbol_set{"x", "z"}) == 0);
        REQUIRE(p_degree(y, symbol_set{"y", "z"}) == 1);
        REQUIRE(p_degree(z, symbol_set{"z"}) == 1);
        REQUIRE(p_degree(z, symbol_set{}) == 0);
        REQUIRE(p_degree(z, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(z, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(z, symbol_set{"x", "y"}) == 0);
        REQUIRE(p_degree(z, symbol_set{"x", "z"}) == 1);
        REQUIRE(p_degree(z, symbol_set{"y", "z"}) == 1);

        REQUIRE(p_degree(x * x, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree(x * x, symbol_set{"x"}) == 2);
        REQUIRE(p_degree(x * x, symbol_set{}) == 0);
        REQUIRE(p_degree(x * x, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(y * x, symbol_set{"y"}) == 1);
        REQUIRE(p_degree(y * x, symbol_set{"y", "x"}) == 2);
        REQUIRE(p_degree(y * x, symbol_set{"x"}) == 1);
        REQUIRE(p_degree(y * x, symbol_set{"z"}) == 0);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"x"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"z"}) == 0);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x", "y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "y", "z"}) == 3);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 1, symbol_set{"x", "y", "z"}) == 3);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 1, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 2, symbol_set{"x", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 2, symbol_set{"y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 3, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 3, symbol_set{}) == 0);
    }

    {
        REQUIRE(p_degree(s11_t{}, symbol_set{}) == 0);
        REQUIRE(p_degree(s11_t{}, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(s11_t{}, symbol_set{"x", "y"}) == 0);

        auto [y, z] = make_polynomials<s11_t>("y", "z");
        auto [x] = make_polynomials<s1_t>("x");
        REQUIRE(std::is_same_v<decltype(p_degree(x, symbol_set{})), int>);
        REQUIRE(std::is_same_v<decltype(p_degree(y, symbol_set{})), int>);
        REQUIRE(std::is_same_v<decltype(p_degree(x * y, symbol_set{})), int>);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(x * 0 + 1, symbol_set{"y", "x"}) == 0);
        REQUIRE(p_degree(x, symbol_set{"x"}) == 1);
        REQUIRE(p_degree(x, symbol_set{"x", "y"}) == 1);
        REQUIRE(p_degree(x, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(x, symbol_set{}) == 0);
        REQUIRE(p_degree(y * 0 + 1, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(y * 0 + 1, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(y * 0 + 1, symbol_set{}) == 0);
        REQUIRE(p_degree(y, symbol_set{"y"}) == 1);
        REQUIRE(p_degree(y, symbol_set{"x"}) == 0);
        REQUIRE(p_degree(y, symbol_set{"x", "y"}) == 1);
        REQUIRE(p_degree(y, symbol_set{}) == 0);
        REQUIRE(p_degree(z, symbol_set{"z"}) == 1);
        REQUIRE(p_degree(z, symbol_set{"z", "x"}) == 1);
        REQUIRE(p_degree(z, symbol_set{"z", "y"}) == 1);
        REQUIRE(p_degree(z, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(z, symbol_set{}) == 0);

        REQUIRE(p_degree(x * x, symbol_set{"x"}) == 2);
        REQUIRE(p_degree(x * x, symbol_set{"y"}) == 0);
        REQUIRE(p_degree(x * x, symbol_set{"y", "x"}) == 2);
        REQUIRE(p_degree(x * x, symbol_set{}) == 0);
        REQUIRE(p_degree(y * x, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree(y * x, symbol_set{"x", "y", "z"}) == 2);
        REQUIRE(p_degree(y * x, symbol_set{"x"}) == 1);
        REQUIRE(p_degree(y * x, symbol_set{"y"}) == 1);
        REQUIRE(p_degree(y * x, symbol_set{"z"}) == 0);
        REQUIRE(p_degree(y * x, symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"x", "y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"x"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{"z"}) == 0);
        REQUIRE(p_degree((x + y) * (x - y), symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x", "y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"x", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - z, symbol_set{}) == 0);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "y", "z"}) == 3);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"x"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y, symbol_set{"y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 1, symbol_set{"x", "y", "z"}) == 3);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 1, symbol_set{"x", "y"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 2, symbol_set{"x", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 2, symbol_set{"y", "z"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 3, symbol_set{"z"}) == 1);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y - 3, symbol_set{"x"}) == 2);
        REQUIRE(p_degree((x + y) * (x - y) - x * z * y + 4, symbol_set{"y"}) == 2);
    }
}

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

namespace piranha::customisation
{

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, ns::s1_t> inline constexpr auto series_div<T, T>
#else
inline constexpr auto series_div<T, T, std::enable_if_t<is_same_cvr_v<T, ns::s1_t>>>
#endif
    = [](const auto &, const auto &) { return false; };

} // namespace piranha::customisation

// Customisation points.
TEST_CASE("series_div_customisation")
{
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

    PIRANHA_REQUIRES_THROWS_CONTAINS(x / 0, mppp::zero_division_error, "");

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
