// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <initializer_list>
#include <limits>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <mp++/rational.hpp>

#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using rat_t = mppp::rational<1>;

using namespace obake;

std::mt19937 rng;

const auto ntrials = 200;

TEST_CASE("series_lookup")
{
    obake_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    for (auto s_idx : {0u, 1u, 2u, 4u}) {
        s1_t s1;
        const auto &s1c = s1;

        s1.set_n_segments(s_idx);

        REQUIRE(std::is_same_v<decltype(s1.find(pm_t{})), s1_t::iterator>);
        REQUIRE(std::is_same_v<decltype(s1c.find(pm_t{})), s1_t::const_iterator>);

        REQUIRE(s1.find(pm_t{}) == s1.end());
        REQUIRE(s1c.find(pm_t{}) == s1.cend());

        REQUIRE(s1.find(pm_t{1, 2, 3}) == s1.end());
        REQUIRE(s1c.find(pm_t{1, 2, 3}) == s1.cend());

        s1.set_symbol_set(symbol_set{"x", "y", "z"});

        s1.add_term(pm_t{1, 2, 3}, "4/5");

        auto it = s1.find(pm_t{1, 2, 3});
        REQUIRE(it != s1.end());
        REQUIRE(it->first == pm_t{1, 2, 3});
        REQUIRE(it->second == rat_t{4, 5});
        it = s1.find(pm_t{-1, 2, 3});
        REQUIRE(it == s1.end());

        auto cit = s1c.find(pm_t{1, 2, 3});
        REQUIRE(cit != s1.cend());
        REQUIRE(cit->first == pm_t{1, 2, 3});
        REQUIRE(cit->second == rat_t{4, 5});
        cit = s1c.find(pm_t{-1, 2, 3});
        REQUIRE(cit == s1.cend());

        s1.add_term(pm_t{4, 5, 6}, -1);

        it = s1.find(pm_t{1, 2, 3});
        REQUIRE(it != s1.end());
        REQUIRE(it->first == pm_t{1, 2, 3});
        REQUIRE(it->second == rat_t{4, 5});
        it = s1.find(pm_t{4, 5, 6});
        REQUIRE(it != s1.end());
        REQUIRE(it->first == pm_t{4, 5, 6});
        REQUIRE(it->second == -1);
        it = s1.find(pm_t{-1, 2, 3});
        REQUIRE(it == s1.end());

        cit = s1c.find(pm_t{1, 2, 3});
        REQUIRE(cit != s1.cend());
        REQUIRE(cit->first == pm_t{1, 2, 3});
        REQUIRE(cit->second == rat_t{4, 5});
        cit = s1c.find(pm_t{4, 5, 6});
        REQUIRE(cit != s1c.cend());
        REQUIRE(cit->first == pm_t{4, 5, 6});
        REQUIRE(cit->second == -1);
        cit = s1c.find(pm_t{-1, 2, 3});
        REQUIRE(cit == s1.cend());

        std::uniform_int_distribution<int> idist(-4, 4), cdist(1, 10);

        for (auto i = 0; i < ntrials; ++i) {
            std::vector<int> tmp_v{idist(rng), idist(rng), idist(rng)};
            const auto cf = cdist(rng);
            s1.add_term(pm_t(tmp_v), cf);

            REQUIRE(s1.find(pm_t(tmp_v)) != s1.end());
            REQUIRE(s1.find(pm_t(tmp_v))->first == pm_t(tmp_v));
            REQUIRE(s1.find(pm_t(tmp_v))->second >= cf);

            REQUIRE(s1c.find(pm_t(tmp_v)) != s1c.end());
            REQUIRE(s1c.find(pm_t(tmp_v))->first == pm_t(tmp_v));
            REQUIRE(s1c.find(pm_t(tmp_v))->second >= cf);
        }
    }
}

TEST_CASE("series_comparison")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using s2_t = series<pm_t, s1_t, void>;

    REQUIRE(!is_equality_comparable_v<s1_t, void>);
    REQUIRE(!is_equality_comparable_v<void, s1_t>);

    for (auto s_idx1 : {0u, 1u, 2u, 4u}) {
        for (auto s_idx2 : {0u, 1u, 2u, 4u}) {
            // Different ranks.
            s1_t s1;
            s1.set_n_segments(s_idx1);

            REQUIRE(s1 == 0);
            REQUIRE(0 == s1);
            REQUIRE(!(s1 != 0));
            REQUIRE(!(0 != s1));

            REQUIRE(!(s1 == 1));
            REQUIRE(!(1 == s1));
            REQUIRE(s1 != 1);
            REQUIRE(1 != s1);

            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s1.add_term(pm_t{}, 5);

            REQUIRE(s1 == 5);
            REQUIRE(5 == s1);
            REQUIRE(!(s1 != 5));
            REQUIRE(!(5 != s1));

            REQUIRE(!(s1 == 3));
            REQUIRE(!(3 == s1));
            REQUIRE(s1 != 3);
            REQUIRE(3 != s1);

            s1 = s1_t{};
            s1.set_symbol_set(symbol_set{"x", "y", "z"});
            s1.set_n_segments(s_idx1);
            s1.add_term(pm_t{1, 2, 3}, 5);

            REQUIRE(!(s1 == 5));
            REQUIRE(!(5 == s1));
            REQUIRE(s1 != 5);
            REQUIRE(5 != s1);

            REQUIRE(!(s1 == 0));
            REQUIRE(!(0 == s1));
            REQUIRE(s1 != 0);
            REQUIRE(0 != s1);

            s2_t s2;
            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s2.set_n_segments(s_idx2);

            REQUIRE(s1 == s2);
            REQUIRE(s2 == s1);
            REQUIRE(!(s1 != s2));
            REQUIRE(!(s2 != s1));

            s2 = s2_t{};
            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s2.set_n_segments(s_idx2);
            s1.add_term(pm_t{}, "4/5");
            s2.add_term(pm_t{}, "4/5");

            REQUIRE(s1 == s2);
            REQUIRE(s2 == s1);
            REQUIRE(!(s1 != s2));
            REQUIRE(!(s2 != s1));

            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s1.add_term(pm_t{}, 1);
            REQUIRE(s1 != s2);
            REQUIRE(s2 != s1);
            REQUIRE(!(s1 == s2));
            REQUIRE(!(s2 == s1));

            // Equal ranks.
            s1_t s1a;
            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s1a.set_n_segments(s_idx2);

            REQUIRE(s1 == s1a);
            REQUIRE(s1a == s1);
            REQUIRE(!(s1 != s1a));
            REQUIRE(!(s1a != s1));

            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s1.add_term(pm_t{}, 4);
            REQUIRE(s1 != s1a);
            REQUIRE(s1a != s1);
            REQUIRE(!(s1 == s1a));
            REQUIRE(!(s1a == s1));

            s1a = s1_t{};
            s1a.set_n_segments(s_idx2);
            s1a.add_term(pm_t{}, -4);
            REQUIRE(s1 != s1a);
            REQUIRE(s1a != s1);
            REQUIRE(!(s1 == s1a));
            REQUIRE(!(s1a == s1));

            s1a = s1;
            REQUIRE(s1 == s1a);
            REQUIRE(s1a == s1);
            REQUIRE(!(s1 != s1a));
            REQUIRE(!(s1a != s1));

            // Identical symbol sets.
            s1a = s1_t{};
            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s1a.set_n_segments(s_idx2);
            s1.set_symbol_set(symbol_set{"x", "y", "z"});
            s1a.set_symbol_set(symbol_set{"x", "y", "z"});
            s1.add_term(pm_t{1, 2, 3}, 5);
            s1a.add_term(pm_t{1, 2, 3}, 5);
            REQUIRE(s1 == s1a);
            REQUIRE(s1a == s1);
            REQUIRE(!(s1 != s1a));
            REQUIRE(!(s1a != s1));
            s1.add_term(pm_t{-1, -2, -3}, -5);
            s1a.add_term(pm_t{-1, -2, -3}, -5);
            REQUIRE(s1 == s1a);
            REQUIRE(s1a == s1);
            REQUIRE(!(s1 != s1a));
            REQUIRE(!(s1a != s1));
            s1.add_term(pm_t{-1, 2, -3}, -5);
            REQUIRE(s1 != s1a);
            REQUIRE(s1a != s1);
            REQUIRE(!(s1 == s1a));
            REQUIRE(!(s1a == s1));

            // Different symbol sets.
            s1a = s1_t{};
            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s1a.set_n_segments(s_idx2);
            s1.set_symbol_set(symbol_set{"x", "y"});
            s1a.set_symbol_set(symbol_set{"x", "y", "z"});
            s1.add_term(pm_t{1, 2}, 5);
            s1a.add_term(pm_t{1, 2, 0}, 5);
            REQUIRE(s1 == s1a);
            REQUIRE(s1a == s1);
            REQUIRE(!(s1 != s1a));
            REQUIRE(!(s1a != s1));
            s1.add_term(pm_t{-1, -2}, -5);
            s1a.add_term(pm_t{-1, -2, 0}, -5);
            REQUIRE(s1 == s1a);
            REQUIRE(s1a == s1);
            REQUIRE(!(s1 != s1a));
            REQUIRE(!(s1a != s1));
            s1.add_term(pm_t{4, 5}, -5);
            s1a.add_term(pm_t{4, 5, 6}, -5);
            REQUIRE(s1 != s1a);
            REQUIRE(s1a != s1);
            REQUIRE(!(s1 == s1a));
            REQUIRE(!(s1a == s1));
            s1a.add_term(pm_t{-4, 5, 6}, -5);
            REQUIRE(s1 != s1a);
            REQUIRE(s1a != s1);
            REQUIRE(!(s1 == s1a));
            REQUIRE(!(s1a == s1));
            s1a = s1_t{};
            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s1a.set_n_segments(s_idx2);
            s1.set_symbol_set(symbol_set{"x", "y"});
            s1a.set_symbol_set(symbol_set{"x", "y", "z"});
            s1.add_term(pm_t{1, 2}, 5);
            REQUIRE(s1 != s1a);
            REQUIRE(s1a != s1);
            REQUIRE(!(s1 == s1a));
            REQUIRE(!(s1a == s1));
        }
    }
}

// Simple testing for in-place add/sub, which are
// currently implemented in terms of the binary
// operators.
TEST_CASE("series_in_place_add_sub")
{
    using pm_t = packed_monomial<int>;
    using s1_t = polynomial<pm_t, rat_t>;
    using s11_t = polynomial<pm_t, s1_t>;

    REQUIRE(!is_in_place_addable_v<s1_t &, void>);
    REQUIRE(!is_in_place_subtractable_v<s1_t &, void>);
    REQUIRE(!is_in_place_addable_v<const s1_t &, int>);
    REQUIRE(!is_in_place_subtractable_v<const s1_t &, int>);
    REQUIRE(!is_in_place_addable_v<void, s1_t &>);
    REQUIRE(!is_in_place_subtractable_v<void, s1_t &>);
    REQUIRE(!is_in_place_addable_v<const int &, s1_t>);
    REQUIRE(!is_in_place_subtractable_v<const int &, s1_t>);
    REQUIRE(!is_in_place_addable_v<const s1_t &, s1_t>);
    REQUIRE(!is_in_place_addable_v<const s1_t &, s11_t>);
    REQUIRE(!is_in_place_addable_v<const s11_t &, s1_t>);
    REQUIRE(!is_in_place_subtractable_v<const s1_t &, s11_t>);
    REQUIRE(!is_in_place_subtractable_v<const s11_t &, s1_t>);

    REQUIRE(std::is_same_v<s1_t &, decltype(std::declval<s1_t &>() += 1)>);
    REQUIRE(std::is_same_v<s1_t &, decltype(std::declval<s1_t &>() -= 1)>);
    REQUIRE(std::is_same_v<s1_t &, decltype(std::declval<s1_t &&>() += 1)>);
    REQUIRE(std::is_same_v<s1_t &, decltype(std::declval<s1_t &&>() -= 1)>);
    REQUIRE(std::is_same_v<int &, decltype(std::declval<int &>() += s1_t{})>);
    REQUIRE(std::is_same_v<int &, decltype(std::declval<int &>() -= s1_t{})>);
    REQUIRE(std::is_same_v<s1_t &, decltype(std::declval<s1_t &>() += s11_t{})>);
    REQUIRE(std::is_same_v<s1_t &, decltype(std::declval<s1_t &&>() += s11_t{})>);
    REQUIRE(std::is_same_v<s11_t &, decltype(std::declval<s11_t &>() += s1_t{})>);
    REQUIRE(std::is_same_v<s11_t &, decltype(std::declval<s11_t &&>() += s1_t{})>);
    REQUIRE(std::is_same_v<s1_t &, decltype(std::declval<s1_t &>() -= s11_t{})>);
    REQUIRE(std::is_same_v<s1_t &, decltype(std::declval<s1_t &&>() -= s11_t{})>);
    REQUIRE(std::is_same_v<s11_t &, decltype(std::declval<s11_t &>() -= s1_t{})>);
    REQUIRE(std::is_same_v<s11_t &, decltype(std::declval<s11_t &&>() -= s1_t{})>);

    for (auto s_idx1 : {0u, 1u, 2u, 4u}) {
        // Scalar.
        s1_t s1;
        s1.set_n_segments(s_idx1);
        s1.add_term(pm_t{}, "4/5");
        s1 += 1;
        REQUIRE(s1 == rat_t{9, 5});
        std::move(s1) += 1;
        REQUIRE(s1 == rat_t{14, 5});

        s1 -= 3;
        REQUIRE(s1 == rat_t{-1, 5});
        std::move(s1) -= 1;
        REQUIRE(s1 == rat_t{-6, 5});

        for (auto s_idx2 : {0u, 1u, 2u, 4u}) {
            // Same rank.
            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s1.set_symbol_set(symbol_set{"x", "y", "z"});
            s1.add_term(pm_t{1, 2, 3}, 1);
            auto old_s1(s1), old_s1_copy(s1);

            s1_t s1a;
            s1a.set_n_segments(s_idx2);
            s1a.set_symbol_set(symbol_set{"x", "y", "z"});
            s1a.add_term(pm_t{4, 5, 6}, 2);
            auto old_s1a(s1a);

            s1 += s1a;
            REQUIRE(s1 == old_s1 + s1a);
            s1 += std::move(s1a);
            REQUIRE(s1 == old_s1 + 2 * old_s1a);

            s1 -= old_s1;
            REQUIRE(s1 == 2 * old_s1a);
            s1 -= std::move(old_s1);
            REQUIRE(s1 == -old_s1_copy + 2 * old_s1a);

            // Do some tests with different symbol sets too,
            // to check the symbol merging.
            auto [a] = make_polynomials<s1_t>("a");
            auto [b] = make_polynomials<s1_t>("b");
            auto a_copy(a);
            auto b_copy(b);
            a += b;
            REQUIRE(a == a_copy + b);
            a += ::std::move(b);
            REQUIRE(a == a_copy + 2 * b_copy);
            b = b_copy;
            a -= b;
            REQUIRE(a == a_copy + b);
            a -= std::move(b);
            REQUIRE(a == a_copy);

            a = make_polynomials<s1_t>(symbol_set{"a", "b"}, "a")[0];
            b = make_polynomials<s1_t>("b")[0];
            a_copy = a;
            b_copy = b;
            a += b;
            REQUIRE(a == a_copy + b);
            a += ::std::move(b);
            REQUIRE(a == a_copy + 2 * b_copy);
            b = b_copy;
            a -= b;
            REQUIRE(a == a_copy + b);
            a -= std::move(b);
            REQUIRE(a == a_copy);

            a = make_polynomials<s1_t>("a")[0];
            b = make_polynomials<s1_t>(symbol_set{"a", "b"}, "b")[0];
            a_copy = a;
            b_copy = b;
            a += b;
            REQUIRE(a == a_copy + b);
            a += ::std::move(b);
            REQUIRE(a == a_copy + 2 * b_copy);
            b = b_copy;
            a -= b;
            REQUIRE(a == a_copy + b);
            a -= std::move(b);
            REQUIRE(a == a_copy);

            a = make_polynomials<s1_t>(symbol_set{"a", "b"}, "a")[0];
            b = make_polynomials<s1_t>(symbol_set{"a", "b"}, "b")[0];
            a_copy = a;
            b_copy = b;
            a += b;
            REQUIRE(a == a_copy + b);
            a += ::std::move(b);
            REQUIRE(a == a_copy + 2 * b_copy);
            b = b_copy;
            a -= b;
            REQUIRE(a == a_copy + b);
            a -= std::move(b);
            REQUIRE(a == a_copy);

            // Run also tests with rvalue reference on the
            // first operand.
            a = make_polynomials<s1_t>(symbol_set{"a", "b"}, "a")[0];
            b = make_polynomials<s1_t>(symbol_set{"a", "b"}, "b")[0];
            a_copy = a;
            b_copy = b;
            ::std::move(a) += b;
            REQUIRE(a == a_copy + b);
            ::std::move(a) += ::std::move(b);
            REQUIRE(a == a_copy + 2 * b_copy);
            b = b_copy;
            ::std::move(a) -= b;
            REQUIRE(a == a_copy + b);
            ::std::move(a) -= std::move(b);
            REQUIRE(a == a_copy);

            a = make_polynomials<s1_t>("a")[0];
            b = make_polynomials<s1_t>(symbol_set{"a", "b"}, "b")[0];
            a_copy = a;
            b_copy = b;
            ::std::move(a) += b;
            REQUIRE(a == a_copy + b);
            ::std::move(a) += ::std::move(b);
            REQUIRE(a == a_copy + 2 * b_copy);
            b = b_copy;
            ::std::move(a) -= b;
            REQUIRE(a == a_copy + b);
            ::std::move(a) -= std::move(b);
            REQUIRE(a == a_copy);

            a = make_polynomials<s1_t>(symbol_set{"a", "b"}, "a")[0];
            b = make_polynomials<s1_t>("b")[0];
            a_copy = a;
            b_copy = b;
            ::std::move(a) += b;
            REQUIRE(a == a_copy + b);
            ::std::move(a) += ::std::move(b);
            REQUIRE(a == a_copy + 2 * b_copy);
            b = b_copy;
            ::std::move(a) -= b;
            REQUIRE(a == a_copy + b);
            ::std::move(a) -= std::move(b);
            REQUIRE(a == a_copy);

            a = make_polynomials<s1_t>("a")[0];
            b = make_polynomials<s1_t>("b")[0];
            a_copy = a;
            b_copy = b;
            ::std::move(a) += b;
            REQUIRE(a == a_copy + b);
            ::std::move(a) += ::std::move(b);
            REQUIRE(a == a_copy + 2 * b_copy);
            b = b_copy;
            ::std::move(a) -= b;
            REQUIRE(a == a_copy + b);
            ::std::move(a) -= std::move(b);
            REQUIRE(a == a_copy);
        }

        // Higher rank vs lower rank.
        auto [a] = make_polynomials<s11_t>("a");
        auto [b] = make_polynomials<s1_t>("b");
        auto a_copy(a);
        auto b_copy(b);
        a += b;
        REQUIRE(a == a_copy + b);
        a += ::std::move(b);
        REQUIRE(a == a_copy + 2 * b_copy);
        b = b_copy;
        a -= b;
        REQUIRE(a == a_copy + b);
        a -= std::move(b);
        REQUIRE(a == a_copy);

        // Try with self.
        s1 = s1_t{};
        s1.set_n_segments(s_idx1);
        s1.set_symbol_set(symbol_set{"x", "y", "z"});
        s1.add_term(pm_t{1, 2, 3}, 1);
        auto old_s1(s1);

        s1 += *&s1;
        REQUIRE(s1 == 2 * old_s1);

        s1 = old_s1;
        s1 += std::move(s1);
        REQUIRE(s1 == 2 * old_s1);

        s1 = old_s1;
        std::move(s1) += s1;
        REQUIRE(s1 == 2 * old_s1);

        s1 = old_s1;
        std::move(s1) += std::move(s1);
        REQUIRE(s1 == 2 * old_s1);

        s1 -= *&s1;
        REQUIRE(s1 == 0);

        s1 = old_s1;
        s1 -= std::move(s1);
        REQUIRE(s1 == 0);

        s1 = old_s1;
        std::move(s1) -= s1;
        REQUIRE(s1 == 0);

        s1 = old_s1;
        std::move(s1) -= std::move(s1);
        REQUIRE(s1 == 0);
    }

    // Scalar on the left.
    for (auto s_idx : {0u, 1u, 2u, 4u}) {
        s1_t s1;
        s1.set_n_segments(s_idx);
        s1.add_term(pm_t{}, 3);
        int n = 5;
        n += s1;

        REQUIRE(n == 8);

        ++n;
        n -= s1;
        REQUIRE(n == 6);

        s1 = s1_t{};
        s1.set_n_segments(s_idx);
        s1.set_symbol_set(symbol_set{"x"});
        s1.add_term(pm_t{1}, 3);

        OBAKE_REQUIRES_THROWS_CONTAINS(n += s1, std::invalid_argument,
                                       "because the series does not consist of a single coefficient");

        OBAKE_REQUIRES_THROWS_CONTAINS(n -= s1, std::invalid_argument,
                                       "because the series does not consist of a single coefficient");
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

inline bool series_mul(const series<pm_t, rat_t, tag00> &, const series<pm_t, rat_t, tag00> &)
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
requires SameCvr<T, ns::s1_t> inline constexpr auto series_mul<T, T>
#else
inline constexpr auto series_mul<T, T, std::enable_if_t<is_same_cvr_v<T, ns::s1_t>>>
#endif
    = [](const auto &, const auto &) { return false; };

} // namespace obake::customisation

// Test for the default series multiplication
// implementation.
TEST_CASE("series_default_mul")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using s1d_t = series<pm_t, double, void>;
    using s2_t = series<pm_t, s1_t, void>;
    using s2d_t = series<pm_t, s1d_t, void>;

    REQUIRE(!is_multipliable_v<s1_t, void>);
    REQUIRE(!is_multipliable_v<void, s1_t>);
    REQUIRE(!is_multipliable_v<s1_t, foo>);
    REQUIRE(!is_multipliable_v<foo, s1_t>);
    REQUIRE(!is_multipliable_v<s1_t, s1_t>);

    for (auto s_idx1 : {0u, 1u, 2u, 4u}) {
        s1_t s1;
        s1.set_n_segments(s_idx1);
        s1.add_term(pm_t{}, "3/4");

        // Multiplication by zero.
        REQUIRE(s1 * 0 == 0);
        REQUIRE(0 * s1 == 0);

        // Simple tests.
        REQUIRE(s1 * 4 == 3);
        REQUIRE(4 * s1 == 3);
        REQUIRE(std::is_same_v<s1_t, decltype(s1 * 4)>);
        REQUIRE(std::is_same_v<s1_t, decltype(4 * s1)>);

        REQUIRE(s1 * 4. == 3.);
        REQUIRE(4. * s1 == 3.);
        REQUIRE(std::is_same_v<s1d_t, decltype(s1 * 4.)>);
        REQUIRE(std::is_same_v<s1d_t, decltype(4. * s1)>);

        s2_t s2;
        s2.set_n_segments(s_idx1);
        s2.add_term(pm_t{}, "3/4");

        REQUIRE(s2 * 0 == 0);
        REQUIRE(0 * s2 == 0);

        REQUIRE(s2 * 4 == 3);
        REQUIRE(4 * s2 == 3);
        REQUIRE(std::is_same_v<s2_t, decltype(s2 * 4)>);
        REQUIRE(std::is_same_v<s2_t, decltype(4 * s2)>);

        REQUIRE(s2 * 4. == 3.);
        REQUIRE(4. * s2 == 3.);
        REQUIRE(std::is_same_v<s2d_t, decltype(s2 * 4.)>);
        REQUIRE(std::is_same_v<s2d_t, decltype(4. * s2)>);

        if (std::numeric_limits<double>::is_iec559) {
            // Try term cancellations.
            s1d_t s1d;
            s1d.set_n_segments(s_idx1);
            s1d.set_symbol_set(symbol_set{"x"});
            s1d.add_term(pm_t{1}, std::numeric_limits<double>::min());
            s1d.add_term(pm_t{2}, std::numeric_limits<double>::min());
            s1d.add_term(pm_t{3}, std::numeric_limits<double>::min());
            s1d.add_term(pm_t{4}, std::numeric_limits<double>::min());

            REQUIRE(s1d * std::numeric_limits<double>::min() == 0.);
            REQUIRE(std::numeric_limits<double>::min() * s1d == 0.);

            s1d.add_term(pm_t{0}, 1);

            REQUIRE(s1d * std::numeric_limits<double>::min() == std::numeric_limits<double>::min());
            REQUIRE(std::numeric_limits<double>::min() * s1d == std::numeric_limits<double>::min());
        }

        s1 = s1_t{};
        s1.set_n_segments(s_idx1);
        s1.add_term(pm_t{}, "3/4");

        s1 *= 2;
        REQUIRE(s1 == rat_t{3, 2});

        std::move(s1) *= 2;
        REQUIRE(s1 == 3);

        int n = 4;
        n *= s1;
        REQUIRE(n == 12);
        n *= std::move(s1);
        REQUIRE(n == 36);
    }

    // Customisation points.
    REQUIRE(series<pm_t, rat_t, ns::tag00>{} * series<pm_t, rat_t, ns::tag00>{});
    REQUIRE(!(ns::s1_t{} * ns::s1_t{}));

    REQUIRE(!is_multipliable_v<series<pm_t, rat_t, ns::tag00>, void>);
    REQUIRE(!is_multipliable_v<void, series<pm_t, rat_t, ns::tag00>>);
    REQUIRE(!is_multipliable_v<ns::s1_t, void>);
    REQUIRE(!is_multipliable_v<void, ns::s1_t>);
}

TEST_CASE("series_typedefs")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    REQUIRE(std::is_same_v<series_term_t<s1_t>, std::pair<const series_key_t<s1_t>, series_cf_t<s1_t>>>);
    REQUIRE(std::is_same_v<series_tag_t<s1_t>, void>);

    REQUIRE(!is_detected_v<series_term_t, void>);
    REQUIRE(!is_detected_v<series_key_t, int>);
    REQUIRE(!is_detected_v<series_cf_t, float>);
    REQUIRE(!is_detected_v<series_tag_t, char>);
}

TEST_CASE("series_clear_terms")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s;
    s.set_n_segments(4);
    s.set_symbol_set(symbol_set{"x", "y", "z"});
    REQUIRE(s.get_symbol_set() == symbol_set{"x", "y", "z"});
    s += 45;
    REQUIRE(s.size() == 1u);
    REQUIRE(s._get_s_table().size() == 16u);

    // Clear terms and check that the series has no terms,
    // but the symbol set and segmentation were not touched.
    s.clear_terms();
    REQUIRE(s.empty());
    REQUIRE(s.get_symbol_set() == symbol_set{"x", "y", "z"});
    REQUIRE(s._get_s_table().size() == 16u);
}

namespace ns
{

// ADL-based customisation.
template <typename K, typename C>
series<K, C, tag00> &series_in_place_add(series<K, C, tag00> &, const series<K, C, tag00> &);

struct tag02 {
};

// Wrong ADL-based customisation.
template <typename K, typename C>
void series_in_place_add(series<K, C, tag02> &, const series<K, C, tag02> &);

} // namespace ns

struct custom_in_place_add {
    template <typename T, typename U>
    T &operator()(T &, const U &) const;
};

struct wrong_custom_in_place_add {
    template <typename T, typename U>
    void operator()(T &, const U &) const;
};

// External customisation.
namespace obake::customisation
{

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, series<ns::pm_t, rat_t, ns::tag01>>
    &&SameCvr<U, series<ns::pm_t, rat_t, ns::tag01>> inline constexpr auto series_in_place_add<T, U>
#else
inline constexpr auto
    series_add<T, U,
               std::enable_if_t<std::conjunction_v<is_same_cvr<T, series<ns::pm_t, rat_t, ns::tag01>>,
                                                   is_same_cvr<U, series<ns::pm_t, rat_t, ns::tag01>>>>>
#endif
    = custom_in_place_add{};

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, series<ns::pm_t, rat_t, ns::tag02>>
    &&SameCvr<U, series<ns::pm_t, rat_t, ns::tag02>> inline constexpr auto series_in_place_add<T, U>
#else
inline constexpr auto
    series_add<T, U,
               std::enable_if_t<std::conjunction_v<is_same_cvr<T, series<ns::pm_t, rat_t, ns::tag02>>,
                                                   is_same_cvr<U, series<ns::pm_t, rat_t, ns::tag02>>>>>
#endif
    = wrong_custom_in_place_add{};

} // namespace obake::customisation

TEST_CASE("series_in_place_add_custom")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, ns::tag00>;
    using s1a_t = series<pm_t, rat_t, ns::tag02>;
    using s2_t = series<ns::pm_t, rat_t, ns::tag01>;
    using s2a_t = series<ns::pm_t, rat_t, ns::tag02>;

    REQUIRE(is_in_place_addable_v<s1_t &, const s1_t &>);
    REQUIRE(!is_in_place_addable_v<s1a_t &, const s1a_t &>);

    REQUIRE(is_in_place_addable_v<s2_t &, const s2_t &>);
    REQUIRE(!is_in_place_addable_v<s2a_t &, const s2a_t &>);
}

namespace ns
{

// ADL-based customisation.
template <typename K, typename C>
series<K, C, tag00> &series_in_place_sub(series<K, C, tag00> &, const series<K, C, tag00> &);

// Wrong ADL-based customisation.
template <typename K, typename C>
void series_in_place_sub(series<K, C, tag02> &, const series<K, C, tag02> &);

} // namespace ns

struct custom_in_place_sub {
    template <typename T, typename U>
    T &operator()(T &, const U &) const;
};

struct wrong_custom_in_place_sub {
    template <typename T, typename U>
    void operator()(T &, const U &) const;
};

// External customisation.
namespace obake::customisation
{

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, series<ns::pm_t, rat_t, ns::tag01>>
    &&SameCvr<U, series<ns::pm_t, rat_t, ns::tag01>> inline constexpr auto series_in_place_sub<T, U>
#else
inline constexpr auto
    series_sub<T, U,
               std::enable_if_t<std::conjunction_v<is_same_cvr<T, series<ns::pm_t, rat_t, ns::tag01>>,
                                                   is_same_cvr<U, series<ns::pm_t, rat_t, ns::tag01>>>>>
#endif
    = custom_in_place_sub{};

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, series<ns::pm_t, rat_t, ns::tag02>>
    &&SameCvr<U, series<ns::pm_t, rat_t, ns::tag02>> inline constexpr auto series_in_place_sub<T, U>
#else
inline constexpr auto
    series_sub<T, U,
               std::enable_if_t<std::conjunction_v<is_same_cvr<T, series<ns::pm_t, rat_t, ns::tag02>>,
                                                   is_same_cvr<U, series<ns::pm_t, rat_t, ns::tag02>>>>>
#endif
    = wrong_custom_in_place_sub{};

} // namespace obake::customisation

TEST_CASE("series_in_place_sub_custom")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, ns::tag00>;
    using s1a_t = series<pm_t, rat_t, ns::tag02>;
    using s2_t = series<ns::pm_t, rat_t, ns::tag01>;
    using s2a_t = series<ns::pm_t, rat_t, ns::tag02>;

    REQUIRE(is_in_place_subtractable_v<s1_t &, const s1_t &>);
    REQUIRE(!is_in_place_subtractable_v<s1a_t &, const s1a_t &>);

    REQUIRE(is_in_place_subtractable_v<s2_t &, const s2_t &>);
    REQUIRE(!is_in_place_subtractable_v<s2a_t &, const s2a_t &>);
}
