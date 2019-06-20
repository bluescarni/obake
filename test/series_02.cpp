// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/series.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <initializer_list>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <mp++/rational.hpp>

#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>

#include "test_utils.hpp"

using rat_t = mppp::rational<1>;

using namespace piranha;

std::mt19937 rng;

const auto ntrials = 200;

TEST_CASE("series_lookup")
{
    piranha_test::disable_slow_stack_traces();

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

// Simple testing for compound add/sub, which are
// currently implemented in terms of the binary
// operators.
TEST_CASE("series_compound_add_sub")
{
    using Catch::Matchers::Contains;

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    REQUIRE(!is_compound_addable_v<s1_t &, void>);
    REQUIRE(!is_compound_subtractable_v<s1_t &, void>);
    REQUIRE(!is_compound_addable_v<const s1_t &, int>);
    REQUIRE(!is_compound_subtractable_v<const s1_t &, int>);

    REQUIRE(!is_compound_addable_v<s1_t &, void>);
    REQUIRE(!is_compound_subtractable_v<s1_t &, void>);
    REQUIRE(!is_compound_addable_v<const int &, s1_t>);
    REQUIRE(!is_compound_subtractable_v<const int &, s1_t>);

    REQUIRE(std::is_same_v<s1_t &, decltype(std::declval<s1_t &>() += 1)>);
    REQUIRE(std::is_same_v<s1_t &, decltype(std::declval<s1_t &>() -= 1)>);
    REQUIRE(std::is_same_v<int &, decltype(std::declval<int &>() += s1_t{})>);
    REQUIRE(std::is_same_v<int &, decltype(std::declval<int &>() -= s1_t{})>);

    for (auto s_idx1 : {0u, 1u, 2u, 4u}) {
        // Scalar.
        s1_t s1;
        s1.set_n_segments(s_idx1);
        s1.add_term(pm_t{}, "4/5");
        s1 += 1;
        REQUIRE(s1 == rat_t{9, 5});

        s1 -= 3;
        REQUIRE(s1 == rat_t{-6, 5});

        for (auto s_idx2 : {0u, 1u, 2u, 4u}) {
            // Same rank.
            s1 = s1_t{};
            s1.set_n_segments(s_idx1);
            s1.set_symbol_set(symbol_set{"x", "y", "z"});
            s1.add_term(pm_t{1, 2, 3}, 1);
            auto old_s1(s1);

            s1_t s1a;
            s1a.set_n_segments(s_idx2);
            s1a.set_symbol_set(symbol_set{"x", "y", "z"});
            s1a.add_term(pm_t{4, 5, 6}, 2);

            s1 += s1a;
            REQUIRE(s1 == old_s1 + s1a);

            s1 -= old_s1;
            REQUIRE(s1 == s1a);
        }
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

        REQUIRE_THROWS_WITH(n += s1, Contains("because the series does not consist of a single coefficient"));
        REQUIRE_THROWS_AS(n += s1, std::invalid_argument);

        REQUIRE_THROWS_WITH(n -= s1, Contains("because the series does not consist of a single coefficient"));
        REQUIRE_THROWS_AS(n -= s1, std::invalid_argument);
    }
}
