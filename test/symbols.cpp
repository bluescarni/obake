// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <initializer_list>
#include <tuple>

#include <piranha/symbols.hpp>

#include "catch.hpp"

using namespace piranha;

TEST_CASE("symbol_set_to_string_test")
{
    REQUIRE(detail::to_string(symbol_set{}) == "{}");
    REQUIRE(detail::to_string(symbol_set{"b"}) == "{'b'}");
    REQUIRE(detail::to_string(symbol_set{"b", "a"}) == "{'a', 'b'}");
    REQUIRE(detail::to_string(symbol_set{"c", "b", "a"}) == "{'a', 'b', 'c'}");
    REQUIRE(detail::to_string(symbol_set{"a", "a", "a"}) == "{'a'}");
}

TEST_CASE("merge_symbol_sets_test")
{
    // The empty test.
    auto ret = detail::merge_symbol_sets(symbol_set{}, symbol_set{});
    REQUIRE(std::get<0>(ret).empty());
    REQUIRE(std::get<1>(ret).empty());
    REQUIRE(std::get<2>(ret).empty());

    // Non-empty vs empty.
    ret = detail::merge_symbol_sets(symbol_set{"a", "b", "c"}, symbol_set{});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c"}));
    REQUIRE(std::get<1>(ret).empty());
    REQUIRE((std::get<2>(ret) == symbol_idx_map<symbol_set>{{0, {"a", "b", "c"}}}));

    // Non-empty vs non-empty.
    ret = detail::merge_symbol_sets(symbol_set{"a", "b", "c"}, symbol_set{"a", "b", "c"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c"}));
    REQUIRE(std::get<1>(ret).empty());
    REQUIRE((std::get<2>(ret).empty()));

    // Empty vs non-empty.
    ret = detail::merge_symbol_sets(symbol_set{}, symbol_set{"a", "b", "c"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c"}));
    REQUIRE((std::get<1>(ret) == symbol_idx_map<symbol_set>{{0, {"a", "b", "c"}}}));
    REQUIRE(std::get<2>(ret).empty());

    // Subsets left.
    ret = detail::merge_symbol_sets(symbol_set{"a", "c"}, symbol_set{"a", "b", "c"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c"}));
    REQUIRE((std::get<1>(ret) == symbol_idx_map<symbol_set>{{1, {"b"}}}));
    REQUIRE(std::get<2>(ret).empty());
    ret = detail::merge_symbol_sets(symbol_set{"a", "b"}, symbol_set{"a", "b", "c"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c"}));
    REQUIRE((std::get<1>(ret) == symbol_idx_map<symbol_set>{{2, {"c"}}}));
    REQUIRE(std::get<2>(ret).empty());
    ret = detail::merge_symbol_sets(symbol_set{"b", "c"}, symbol_set{"a", "b", "c"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c"}));
    REQUIRE((std::get<1>(ret) == symbol_idx_map<symbol_set>{{0, {"a"}}}));
    REQUIRE(std::get<2>(ret).empty());

    // Subsets right.
    ret = detail::merge_symbol_sets(symbol_set{"a", "b", "c"}, symbol_set{"a", "c"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c"}));
    REQUIRE(std::get<1>(ret).empty());
    REQUIRE((std::get<2>(ret) == symbol_idx_map<symbol_set>{{1, {"b"}}}));
    ret = detail::merge_symbol_sets(symbol_set{"a", "b", "c"}, symbol_set{"a", "b"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c"}));
    REQUIRE(std::get<1>(ret).empty());
    REQUIRE((std::get<2>(ret) == symbol_idx_map<symbol_set>{{2, {"c"}}}));
    ret = detail::merge_symbol_sets(symbol_set{"a", "b", "c"}, symbol_set{"b", "c"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c"}));
    REQUIRE(std::get<1>(ret).empty());
    REQUIRE((std::get<2>(ret) == symbol_idx_map<symbol_set>{{0, {"a"}}}));

    // Disjoint.
    ret = detail::merge_symbol_sets(symbol_set{"a", "b", "c"}, symbol_set{"d", "e", "f"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c", "d", "e", "f"}));
    REQUIRE((std::get<1>(ret) == symbol_idx_map<symbol_set>{{3, {"d", "e", "f"}}}));
    REQUIRE((std::get<2>(ret) == symbol_idx_map<symbol_set>{{0, {"a", "b", "c"}}}));
    ret = detail::merge_symbol_sets(symbol_set{"d", "e", "f"}, symbol_set{"a", "b", "c"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c", "d", "e", "f"}));
    REQUIRE((std::get<1>(ret) == symbol_idx_map<symbol_set>{{0, {"a", "b", "c"}}}));
    REQUIRE((std::get<2>(ret) == symbol_idx_map<symbol_set>{{3, {"d", "e", "f"}}}));

    // Misc.
    ret = detail::merge_symbol_sets(symbol_set{"b", "c", "e"}, symbol_set{"a", "c", "d", "f", "g"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c", "d", "e", "f", "g"}));
    REQUIRE((std::get<1>(ret) == symbol_idx_map<symbol_set>{{0, {"a"}}, {2, {"d"}}, {3, {"f", "g"}}}));
    REQUIRE((std::get<2>(ret) == symbol_idx_map<symbol_set>{{1, {"b"}}, {3, {"e"}}}));
    ret = detail::merge_symbol_sets(symbol_set{"b", "n", "t", "z"}, symbol_set{"a", "c", "d", "f", "g", "m", "o", "x"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c", "d", "f", "g", "m", "n", "o", "t", "x", "z"}));
    REQUIRE((std::get<1>(ret)
             == symbol_idx_map<symbol_set>{{0, {"a"}}, {1, {"c", "d", "f", "g", "m"}}, {2, {"o"}}, {3, {"x"}}}));
    REQUIRE((std::get<2>(ret) == symbol_idx_map<symbol_set>{{1, {"b"}}, {6, {"n"}}, {7, {"t"}}, {8, {"z"}}}));
    ret = detail::merge_symbol_sets(symbol_set{"b", "n", "t"}, symbol_set{"a", "c", "d", "f", "g", "m", "o", "x"});
    REQUIRE((std::get<0>(ret) == symbol_set{"a", "b", "c", "d", "f", "g", "m", "n", "o", "t", "x"}));
    REQUIRE((std::get<1>(ret)
             == symbol_idx_map<symbol_set>{{0, {"a"}}, {1, {"c", "d", "f", "g", "m"}}, {2, {"o"}}, {3, {"x"}}}));
    REQUIRE((std::get<2>(ret) == symbol_idx_map<symbol_set>{{1, {"b"}}, {6, {"n"}}, {7, {"t"}}}));
}

TEST_CASE("ss_intersect_idx_test")
{
    REQUIRE(detail::ss_intersect_idx(symbol_set{}, symbol_set{}).size() == 0u);
    REQUIRE(detail::ss_intersect_idx(symbol_set{"a"}, symbol_set{}).size() == 0u);
    REQUIRE(detail::ss_intersect_idx(symbol_set{"a", "b", "c"}, symbol_set{}).size() == 0u);
    REQUIRE(detail::ss_intersect_idx(symbol_set{"b", "c"}, symbol_set{"d"}).size() == 0u);
    REQUIRE(detail::ss_intersect_idx(symbol_set{"b", "c"}, symbol_set{"a"}).size() == 0u);
    REQUIRE(detail::ss_intersect_idx(symbol_set{"a", "b", "c"}, symbol_set{"a"}) == symbol_idx_set{0});
    REQUIRE(detail::ss_intersect_idx(symbol_set{"a", "b", "c"}, symbol_set{"b"}) == symbol_idx_set{0});
    REQUIRE(detail::ss_intersect_idx(symbol_set{"a", "b", "c"}, symbol_set{"c"}) == symbol_idx_set{0});
    REQUIRE(detail::ss_intersect_idx(symbol_set{"a"}, symbol_set{"a", "b", "c"}) == symbol_idx_set{0});
    REQUIRE(detail::ss_intersect_idx(symbol_set{"b"}, symbol_set{"a", "b", "c"}) == symbol_idx_set{1});
    REQUIRE(detail::ss_intersect_idx(symbol_set{"c"}, symbol_set{"a", "b", "c"}) == symbol_idx_set{2});
    REQUIRE((detail::ss_intersect_idx(symbol_set{"a", "b", "c", "d", "g"}, symbol_set{"b", "d", "e"})
             == symbol_idx_set{0, 1}));
    REQUIRE((detail::ss_intersect_idx(symbol_set{"b", "d", "e"}, symbol_set{"a", "b", "c", "d", "g"})
             == symbol_idx_set{1, 3}));
    REQUIRE(
        (detail::ss_intersect_idx(symbol_set{"a", "b", "c", "d", "g"}, symbol_set{"x", "y", "z"}) == symbol_idx_set{}));
    REQUIRE(
        (detail::ss_intersect_idx(symbol_set{"x", "y", "z"}, symbol_set{"a", "b", "c", "d", "g"}) == symbol_idx_set{}));
    REQUIRE((detail::ss_intersect_idx(symbol_set{"c", "d", "g"}, symbol_set{"a", "b", "e"}) == symbol_idx_set{}));
    REQUIRE((detail::ss_intersect_idx(symbol_set{"a", "b", "e"}, symbol_set{"c", "d", "g"}) == symbol_idx_set{}));
    REQUIRE((detail::ss_intersect_idx(symbol_set{"c", "e", "g"}, symbol_set{"a", "b", "e"}) == symbol_idx_set{2}));
    REQUIRE((detail::ss_intersect_idx(symbol_set{"a", "b", "e"}, symbol_set{"c", "e", "g"}) == symbol_idx_set{1}));
    REQUIRE(
        (detail::ss_intersect_idx(symbol_set{"c", "e", "g"}, symbol_set{"c", "e", "g"}) == symbol_idx_set{0, 1, 2}));
}
