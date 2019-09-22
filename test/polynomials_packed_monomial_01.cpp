// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cmath>
#include <initializer_list>
#include <tuple>
#include <type_traits>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <obake/config.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/key/key_evaluate.hpp>
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

TEST_CASE("key_evaluate_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_evaluable_key_v<pm_t, double>);
        REQUIRE(is_evaluable_key_v<pm_t &, double>);
        REQUIRE(is_evaluable_key_v<const pm_t &, double>);
        REQUIRE(is_evaluable_key_v<const pm_t, double>);
        REQUIRE(!is_evaluable_key_v<const pm_t, double &>);
        REQUIRE(!is_evaluable_key_v<const pm_t, void>);
        REQUIRE(std::is_same_v<double, decltype(key_evaluate(pm_t{}, symbol_idx_map<double>{}, symbol_set{}))>);
        REQUIRE(key_evaluate(pm_t{}, symbol_idx_map<double>{}, symbol_set{}) == 1.);
        REQUIRE(key_evaluate(pm_t{2}, symbol_idx_map<double>{{0, 3.5}}, symbol_set{"x"}) == std::pow(3.5, 2.));
        REQUIRE(key_evaluate(pm_t{2, 3}, symbol_idx_map<double>{{0, 3.5}, {1, -4.6}}, symbol_set{"x", "y"})
                == std::pow(3.5, 2.) * std::pow(-4.6, 3));

        if constexpr (is_signed_v<int_t>) {
            REQUIRE(key_evaluate(pm_t{-2, 3}, symbol_idx_map<double>{{0, 3.5}, {1, -4.6}}, symbol_set{"x", "y"})
                    == std::pow(3.5, -2.) * std::pow(-4.6, 3));
        }

        REQUIRE(!is_evaluable_key_v<pm_t, int>);

        REQUIRE(is_evaluable_key_v<pm_t, mppp::integer<1>>);
        REQUIRE(std::is_same_v<mppp::integer<1>,
                               decltype(key_evaluate(pm_t{}, symbol_idx_map<mppp::integer<1>>{}, symbol_set{}))>);
        REQUIRE(key_evaluate(pm_t{}, symbol_idx_map<mppp::integer<1>>{}, symbol_set{}) == 1);
        REQUIRE(key_evaluate(pm_t{2}, symbol_idx_map<mppp::integer<1>>{{0, mppp::integer<1>{3}}}, symbol_set{"x"})
                == mppp::pow(mppp::integer<1>{3}, 2));
        REQUIRE(key_evaluate(pm_t{2, 3},
                             symbol_idx_map<mppp::integer<1>>{{0, mppp::integer<1>{3}}, {1, mppp::integer<1>{4}}},
                             symbol_set{"x", "y"})
                == 576);

        if constexpr (is_signed_v<int_t>) {
            REQUIRE(key_evaluate(pm_t{-2, 3},
                                 symbol_idx_map<mppp::integer<1>>{{0, mppp::integer<1>{3}}, {1, mppp::integer<1>{4}}},
                                 symbol_set{"x", "y"})
                    == 0);
        }

        REQUIRE(is_evaluable_key_v<pm_t, mppp::rational<1>>);
        REQUIRE(std::is_same_v<mppp::rational<1>,
                               decltype(key_evaluate(pm_t{}, symbol_idx_map<mppp::rational<1>>{}, symbol_set{}))>);
    });
}
