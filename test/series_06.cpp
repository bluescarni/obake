// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <mp++/config.hpp>
#include <mp++/exceptions.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#if defined(MPPP_WITH_MPFR)
#include <mp++/real.hpp>
#endif

#include <obake/config.hpp>
#include <obake/key/key_degree.hpp>
#include <obake/math/degree.hpp>
#include <obake/math/pow.hpp>
#include <obake/polynomials/d_packed_monomial.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using int_t = mppp::integer<1>;
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
    REQUIRE(!is_in_place_divisible_v<s1_t &, const s1_t &>);
    REQUIRE(!is_in_place_divisible_v<s1_t &, void>);
    REQUIRE(!is_in_place_divisible_v<s11_t &, const s11_t &>);
    REQUIRE(!is_in_place_divisible_v<s11_t &, const s1_t &>);
    REQUIRE(!is_in_place_divisible_v<int &, const s1_t &>);
}

TEST_CASE("series_conversion_operator")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1{"3/4"};
    REQUIRE(static_cast<rat_t>(s1) == rat_t{3, 4});
    REQUIRE(static_cast<double>(s1) == 3 / 4.);

    REQUIRE(static_cast<rat_t>(s1_t{}) == 0);
    REQUIRE(static_cast<int>(s1_t{}) == 0);

    s1 = s1_t{};
    s1.set_n_segments(1);
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, 1);
    s1.add_term(pm_t{-1, -2, -3}, -1);
    s1.add_term(pm_t{4, 5, 6}, 2);
    s1.add_term(pm_t{7, 8, 9}, -2);
    OBAKE_REQUIRES_THROWS_CONTAINS((void)static_cast<rat_t>(s1), std::invalid_argument,
                                   "because the series does not consist of a single coefficient");

    // Bug: conversion would succeed in case a single
    // term with non-unitary key was present.
    s1 = s1_t{};
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, 1);
    OBAKE_REQUIRES_THROWS_CONTAINS((void)static_cast<rat_t>(s1), std::invalid_argument,
                                   "because the series does not consist of a single coefficient");
}

template <typename T, typename F>
using filtered_t = decltype(filtered(std::declval<T>(), std::declval<F>()));

TEST_CASE("series_filtered_test")
{
    using pm_t = packed_monomial<int>;
    using p1_t = polynomial<pm_t, rat_t>;

    REQUIRE(filtered(p1_t{}, [](const auto &) { return true; }).empty());

    p1_t tmp;
    tmp.set_symbol_set(symbol_set{"a", "b", "c"});
    tmp.set_n_segments(4);

    auto tmp_f = filtered(tmp, [](const auto &) { return true; });
    REQUIRE(tmp_f.empty());
    REQUIRE(tmp_f.get_symbol_set() == symbol_set{"a", "b", "c"});
    REQUIRE(tmp_f._get_s_table().size() == 16u);

    auto [x, y, z] = make_polynomials<p1_t>("x", "y", "z");

    auto p = obake::pow(1 + x + y + z, 4);
    auto pf = filtered(p, [&ss = p.get_symbol_set()](const auto &t) { return obake::key_degree(t.first, ss) <= 1; });
    REQUIRE(obake::degree(pf) == 1);
    pf = filtered(p, [&ss = p.get_symbol_set()](const auto &t) { return obake::key_degree(t.first, ss) <= 2; });
    REQUIRE(obake::degree(pf) == 2);
    pf = filtered(p, [&ss = p.get_symbol_set()](const auto &t) { return obake::key_degree(t.first, ss) <= 3; });
    REQUIRE(obake::degree(pf) == 3);
    REQUIRE(pf.get_symbol_set() == symbol_set{"x", "y", "z"});

    REQUIRE(!is_detected_v<filtered_t, void, void>);
    REQUIRE(!is_detected_v<filtered_t, void, int>);
    REQUIRE(!is_detected_v<filtered_t, int, void>);
    auto good_f = [](const auto &) { return true; };
    REQUIRE(!is_detected_v<filtered_t, int, decltype(good_f)>);
    REQUIRE(is_detected_v<filtered_t, p1_t, decltype(good_f)>);
    REQUIRE(is_detected_v<filtered_t, p1_t, decltype(good_f) &>);
    REQUIRE(is_detected_v<filtered_t, p1_t, const decltype(good_f) &>);
    auto good_f1 = [](const auto &) { return 1; };
    REQUIRE(is_detected_v<filtered_t, p1_t, decltype(good_f1)>);
    REQUIRE(is_detected_v<filtered_t, p1_t, decltype(good_f1) &>);
    REQUIRE(is_detected_v<filtered_t, p1_t, const decltype(good_f1) &>);
    auto bad_f0 = []() { return true; };
    REQUIRE(!is_detected_v<filtered_t, p1_t, decltype(bad_f0)>);
    auto bad_f1 = [](const auto &) {};
    REQUIRE(!is_detected_v<filtered_t, p1_t, decltype(bad_f1)>);
    struct foo {
    };
    auto bad_f2 = [](const auto &) { return foo{}; };
    REQUIRE(!is_detected_v<filtered_t, p1_t, decltype(bad_f2)>);
}

TEST_CASE("series_generic_ctor_with_ss")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using s1_int_t = series<pm_t, int_t, void>;
    using s2_t = series<pm_t, s1_t, void>;

#if defined(MPPP_WITH_MPFR)
    using s1_real_t = series<pm_t, mppp::real, void>;
#endif

    REQUIRE(!std::is_constructible_v<s1_t, void, void>);
    REQUIRE(!std::is_constructible_v<s1_t, int, void>);
    REQUIRE(!std::is_constructible_v<s1_t, void, int>);

    // Constructability from non-series type.
    REQUIRE(std::is_constructible_v<s1_t, int, symbol_set>);
    REQUIRE(std::is_constructible_v<s1_t, int &, const symbol_set &>);
    REQUIRE(std::is_constructible_v<s1_t, const int &, symbol_set &>);

    s1_t s1{5, symbol_set{}};
    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == 5);
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

    s1 = s1_t{0., symbol_set{"x"}};
    REQUIRE(s1.empty());
    REQUIRE(s1.get_symbol_set() == symbol_set{"x"});

    s1 = s1_t{"3/4", symbol_set{"x", "y"}};
    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(s1.begin()->second == rat_t{3, 4});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{"x", "y"}));

    s2_t s2{5, symbol_set{"x", "y"}};
    REQUIRE(s2.size() == 1u);
    REQUIRE(s2.get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{"x", "y"}));
    s1 = s2.begin()->second;
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == 5);
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

    s2 = s2_t{0, symbol_set{"x", "y"}};
    REQUIRE(s2.empty());
    REQUIRE(s2.get_symbol_set() == symbol_set{"x", "y"});

    s2 = s2_t{"3/4", symbol_set{"x", "y"}};
    REQUIRE(s2.size() == 1u);
    REQUIRE(s2.get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{"x", "y"}));
    s1 = s2.begin()->second;
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == rat_t{3, 4});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

    // Constructability from lower rank series.
    REQUIRE(std::is_constructible_v<s2_t, s1_t, symbol_set>);
    REQUIRE(std::is_constructible_v<s2_t, s1_t &, symbol_set &>);
    REQUIRE(std::is_constructible_v<s2_t, const s1_t &, const symbol_set &>);

    s2 = s2_t{s1_t{5}, symbol_set{"x", "y"}};
    REQUIRE(s2.size() == 1u);
    REQUIRE(s2.get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{"x", "y"}));
    s1 = s2.begin()->second;
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == 5);
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

    s2 = s2_t{s1_t{0}, symbol_set{"x", "y"}};
    REQUIRE(s2.empty());
    REQUIRE(s2.get_symbol_set() == symbol_set{"x", "y"});

    s2 = s2_t{s1_t{"3/4"}, symbol_set{"x", "y"}};
    REQUIRE(s2.size() == 1u);
    REQUIRE(s2.get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{"x", "y"}));
    s1 = s2.begin()->second;
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == rat_t{3, 4});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

#if defined(MPPP_WITH_MPFR)

    // Verify that move construction moves.
    mppp::real r{42};
    s1_real_t s1r{std::move(r), symbol_set{"x", "y"}};
    REQUIRE(r._get_mpfr_t()->_mpfr_d == nullptr);

#endif

    // Non-constructability from equal rank series.
    REQUIRE(!std::is_constructible_v<s1_t, s1_int_t, symbol_set>);
    REQUIRE(!std::is_constructible_v<s1_t, s1_int_t &, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<s1_t, const s1_int_t &, symbol_set &>);
    REQUIRE(!std::is_constructible_v<s1_t, series<pm_t, rat_t, int>, symbol_set>);

    // Non-constructability from a series with higher rank.
    REQUIRE(!std::is_constructible_v<s1_t, s2_t, symbol_set>);
    REQUIRE(!std::is_constructible_v<s1_t, s2_t &, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<s1_t, const s2_t &, symbol_set &>);
}

TEST_CASE("series_generic_ctor_with_ss_bug_00")
{
    // The constructor would not create a key
    // compatible with the input symbol set.
    using pm_t = d_packed_monomial<int, 8>;
    using s1_t = series<pm_t, rat_t, void>;

    REQUIRE(s1_t(42, symbol_set{"x", "y", "z"}) == 42);
    REQUIRE(s1_t(42, symbol_set{"x", "y", "z"}).get_symbol_set() == symbol_set{"x", "y", "z"});
}
