// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <initializer_list>
#include <sstream>
#include <tuple>

#include <obake/config.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/key/key_tex_stream_insert.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

using int_types = std::tuple<int, unsigned, long, unsigned long, long long, unsigned long long
// NOTE: clang + ubsan fail to compile with 128bit integers in this test.
#if defined(OBAKE_HAVE_GCC_INT128) && !defined(OBAKE_TEST_CLANG_UBSAN)
                             ,
                             __int128_t, __uint128_t
#endif
                             >;

TEST_CASE("key_stream_insert_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_tex_stream_insertable_key_v<pm_t>);
        REQUIRE(is_tex_stream_insertable_key_v<pm_t &>);
        REQUIRE(is_tex_stream_insertable_key_v<const pm_t &>);
        REQUIRE(is_tex_stream_insertable_key_v<const pm_t>);

        std::ostringstream oss;

        key_tex_stream_insert(oss, pm_t{}, symbol_set{});
        REQUIRE(oss.str().empty());
        oss.str("");

        key_tex_stream_insert(oss, pm_t{1}, symbol_set{"x"});
        REQUIRE(oss.str() == "{x}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{1, 2}, symbol_set{"x", "y"});
        REQUIRE(oss.str() == "{x}{y}^{2}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{0, 2}, symbol_set{"x", "y"});
        REQUIRE(oss.str() == "{y}^{2}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{1, 0}, symbol_set{"x", "y"});
        REQUIRE(oss.str() == "{x}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{2, 0}, symbol_set{"x", "y"});
        REQUIRE(oss.str() == "{x}^{2}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{2, 0, 1}, symbol_set{"x", "y", "z"});
        REQUIRE(oss.str() == "{x}^{2}{z}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{1, 2, 3}, symbol_set{"x", "y", "z"});
        REQUIRE(oss.str() == "{x}{y}^{2}{z}^{3}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{0, 0, 1}, symbol_set{"x", "y", "z"});
        REQUIRE(oss.str() == "{z}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{0, 0, 4}, symbol_set{"x", "y", "z"});
        REQUIRE(oss.str() == "{z}^{4}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{0, 0, 0}, symbol_set{"x", "y", "z"});
        REQUIRE(oss.str().empty());
        oss.str("");

        if constexpr (is_signed_v<int_t>) {
            key_tex_stream_insert(oss, pm_t{-1}, symbol_set{"x"});
            REQUIRE(oss.str() == "\\frac{1}{{x}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{-1, -2}, symbol_set{"x", "y"});
            REQUIRE(oss.str() == "\\frac{1}{{x}{y}^{2}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{0, -2}, symbol_set{"x", "y"});
            REQUIRE(oss.str() == "\\frac{1}{{y}^{2}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{-1, 0}, symbol_set{"x", "y"});
            REQUIRE(oss.str() == "\\frac{1}{{x}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{-1, -2, -3}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{1}{{x}{y}^{2}{z}^{3}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{1, -2, -3}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{{x}}{{y}^{2}{z}^{3}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{2, -2, -3}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{{x}^{2}}{{y}^{2}{z}^{3}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{2, -2, 3}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{{x}^{2}{z}^{3}}{{y}^{2}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{-2, -2, 3}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{{z}^{3}}{{x}^{2}{y}^{2}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{-2, 0, 0}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{1}{{x}^{2}}");
            oss.str("");
        }
    });
}
