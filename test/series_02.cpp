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
#include <type_traits>
#include <vector>

#include <mp++/rational.hpp>

#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/symbols.hpp>

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
