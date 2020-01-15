// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <exception>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#if defined(MPPP_WITH_MPFR)
#include <mp++/real.hpp>
#endif

#include <obake/config.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

using int_t = mppp::integer<1>;
using rat_t = mppp::rational<1>;

#if defined(MPPP_WITH_MPFR)

using real = mppp::real;

#endif

TEST_CASE("cf_key_concepts")
{
    obake_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<int>;

    REQUIRE(!is_cf_v<void>);
    REQUIRE(!is_key_v<void>);

    REQUIRE(is_cf_v<int>);
    REQUIRE(is_cf_v<double>);
    REQUIRE(!is_cf_v<const double>);
    REQUIRE(!is_cf_v<double &>);
    REQUIRE(!is_cf_v<const double &>);
    REQUIRE(!is_cf_v<double &&>);

    REQUIRE(is_key_v<pm_t>);
    REQUIRE(!is_key_v<const pm_t>);
    REQUIRE(!is_key_v<pm_t &>);
    REQUIRE(!is_key_v<const pm_t &>);
    REQUIRE(!is_key_v<pm_t &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Cf<void>);
    REQUIRE(!Key<void>);

    REQUIRE(Cf<int>);
    REQUIRE(Cf<double>);
    REQUIRE(!Cf<const double>);
    REQUIRE(!Cf<double &>);
    REQUIRE(!Cf<const double &>);
    REQUIRE(!Cf<double &&>);

    REQUIRE(Key<pm_t>);
    REQUIRE(!Key<const pm_t>);
    REQUIRE(!Key<pm_t &>);
    REQUIRE(!Key<const pm_t &>);
    REQUIRE(!Key<pm_t &&>);
#endif
}

TEST_CASE("series_rank")
{
    using pm_t = packed_monomial<int>;
    using series_t = series<pm_t, rat_t, void>;
    using series2_t = series<pm_t, series_t, void>;

    REQUIRE(series_rank<void> == 0u);

    REQUIRE(series_rank<series_t> == 1u);
    REQUIRE(series_rank<series_t &> == 0u);
    REQUIRE(series_rank<const series_t> == 0u);
    REQUIRE(series_rank<const series_t &> == 0u);
    REQUIRE(series_rank<series_t &&> == 0u);

    REQUIRE(series_rank<series2_t> == 2u);
    REQUIRE(series_rank<series2_t &> == 0u);
    REQUIRE(series_rank<const series2_t> == 0u);
    REQUIRE(series_rank<const series2_t &> == 0u);
    REQUIRE(series_rank<series2_t &&> == 0u);
}

TEST_CASE("series_cf_key_tag_t")
{
    using pm_t = packed_monomial<int>;
    using series_t = series<pm_t, rat_t, void>;

    REQUIRE(std::is_same_v<pm_t, series_key_t<series_t>>);
    REQUIRE(std::is_same_v<rat_t, series_cf_t<series_t>>);
    REQUIRE(std::is_same_v<void, series_tag_t<series_t>>);

    REQUIRE(!is_detected_v<series_key_t, const series_t>);
    REQUIRE(!is_detected_v<series_cf_t, series_t &>);
    REQUIRE(!is_detected_v<series_key_t, const series_t &&>);
}

TEST_CASE("is_cvr_series")
{
    using pm_t = packed_monomial<int>;
    using series_t = series<pm_t, rat_t, void>;

    REQUIRE(!is_cvr_series_v<void>);
    REQUIRE(!is_cvr_series_v<int>);
    REQUIRE(!is_cvr_series_v<double>);

    REQUIRE(is_cvr_series_v<series_t>);
    REQUIRE(is_cvr_series_v<series_t &>);
    REQUIRE(is_cvr_series_v<const series_t &>);
    REQUIRE(is_cvr_series_v<series_t &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!CvrSeries<void>);
    REQUIRE(!CvrSeries<int>);
    REQUIRE(!CvrSeries<double>);

    REQUIRE(CvrSeries<series_t>);
    REQUIRE(CvrSeries<series_t &>);
    REQUIRE(CvrSeries<const series_t &>);
    REQUIRE(CvrSeries<series_t &&>);
#endif
}

TEST_CASE("add_term_primitives")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    using detail::sat_assume_unique;
    using detail::sat_check_compat_key;
    using detail::sat_check_table_size;
    using detail::sat_check_zero;

    const auto vt = std::make_tuple(std::false_type{}, std::true_type{});

    detail::tuple_for_each(vt, [&](auto v_sign) {
        detail::tuple_for_each(vt, [&](auto v_cz) {
            detail::tuple_for_each(vt, [&](auto v_cck) {
                detail::tuple_for_each(vt, [&](auto v_cts) {
                    detail::tuple_for_each(vt, [&](auto v_au) {
                        // Insertion with an rvalue.
                        s1_t s1;
                        s1.set_symbol_set(symbol_set{"x", "y", "z"});
                        detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                      static_cast<sat_check_compat_key>(v_cck.value),
                                                      static_cast<sat_check_table_size>(v_cts.value),
                                                      static_cast<sat_assume_unique>(v_au.value)>(
                            s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{42});
                        REQUIRE(s1.size() == 1u);
                        REQUIRE(s1.begin()->first == pm_t{1, 2, 3});
                        if (v_sign.value) {
                            REQUIRE(s1.begin()->second == 42);
                        } else {
                            REQUIRE(s1.begin()->second == -42);
                        }

                        // Insertion with an lvalue.
                        rat_t q{42, 13};
                        s1 = s1_t{};
                        s1.set_symbol_set(symbol_set{"x", "y", "z"});
                        detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                      static_cast<sat_check_compat_key>(v_cck.value),
                                                      static_cast<sat_check_table_size>(v_cts.value),
                                                      static_cast<sat_assume_unique>(v_au.value)>(
                            s1, s1._get_s_table()[0], pm_t{1, 2, 3}, q);
                        REQUIRE(s1.size() == 1u);
                        REQUIRE(s1.begin()->first == pm_t{1, 2, 3});
                        if (v_sign.value) {
                            REQUIRE(s1.begin()->second == q);
                        } else {
                            REQUIRE(s1.begin()->second == -q);
                        }

                        // Insertion with a single argument that can be used to
                        // construct a coefficient.
                        s1 = s1_t{};
                        s1.set_symbol_set(symbol_set{"x", "y", "z"});
                        detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                      static_cast<sat_check_compat_key>(v_cck.value),
                                                      static_cast<sat_check_table_size>(v_cts.value),
                                                      static_cast<sat_assume_unique>(v_au.value)>(
                            s1, s1._get_s_table()[0], pm_t{1, 2, 3}, 42);
                        REQUIRE(s1.size() == 1u);
                        REQUIRE(s1.begin()->first == pm_t{1, 2, 3});
                        if (v_sign.value) {
                            REQUIRE(s1.begin()->second == 42);
                        } else {
                            REQUIRE(s1.begin()->second == -42);
                        }

                        // Insertion with multiple arguments that can be used to
                        // construct a coefficient.
                        s1 = s1_t{};
                        s1.set_symbol_set(symbol_set{"x", "y", "z"});
                        detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                      static_cast<sat_check_compat_key>(v_cck.value),
                                                      static_cast<sat_check_table_size>(v_cts.value),
                                                      static_cast<sat_assume_unique>(v_au.value)>(
                            s1, s1._get_s_table()[0], pm_t{1, 2, 3}, 42, 13);
                        REQUIRE(s1.size() == 1u);
                        REQUIRE(s1.begin()->first == pm_t{1, 2, 3});
                        if (v_sign.value) {
                            REQUIRE(s1.begin()->second == q);
                        } else {
                            REQUIRE(s1.begin()->second == -q);
                        }

                        // Same pattern as above, but make sure that coefficient add/sub happens.
                        // NOTE: test meaningful only if we are not assuming unique.
                        if (!v_au()) {
                            s1 = s1_t{};
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{42});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{-6});
                            REQUIRE(s1.size() == 1u);
                            REQUIRE(s1.begin()->first == pm_t{1, 2, 3});
                            if (v_sign.value) {
                                REQUIRE(s1.begin()->second == 36);
                            } else {
                                REQUIRE(s1.begin()->second == -36);
                            }

                            s1 = s1_t{};
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{42});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, q);
                            REQUIRE(s1.size() == 1u);
                            REQUIRE(s1.begin()->first == pm_t{1, 2, 3});
                            if (v_sign.value) {
                                REQUIRE(s1.begin()->second == rat_t{588, 13});
                            } else {
                                REQUIRE(s1.begin()->second == -rat_t{588, 13});
                            }

                            s1 = s1_t{};
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{42});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, 1);
                            REQUIRE(s1.size() == 1u);
                            REQUIRE(s1.begin()->first == pm_t{1, 2, 3});
                            if (v_sign.value) {
                                REQUIRE(s1.begin()->second == 43);
                            } else {
                                REQUIRE(s1.begin()->second == -43);
                            }

                            s1 = s1_t{};
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{42});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, 42, 13);
                            REQUIRE(s1.size() == 1u);
                            REQUIRE(s1.begin()->first == pm_t{1, 2, 3});
                            if (v_sign.value) {
                                REQUIRE(s1.begin()->second == rat_t{588, 13});
                            } else {
                                REQUIRE(s1.begin()->second == -rat_t{588, 13});
                            }
                        }

                        // Check term annihilation or zero insertion.
                        if (v_cz()) {
                            s1 = s1_t{};
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{0});
                            REQUIRE(s1.empty());

                            if (!v_au()) {
                                s1 = s1_t{};
                                s1.set_symbol_set(symbol_set{"x", "y", "z"});
                                detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                              static_cast<sat_check_compat_key>(v_cck.value),
                                                              static_cast<sat_check_table_size>(v_cts.value),
                                                              static_cast<sat_assume_unique>(v_au.value)>(
                                    s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{42});
                                detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                              static_cast<sat_check_compat_key>(v_cck.value),
                                                              static_cast<sat_check_table_size>(v_cts.value),
                                                              static_cast<sat_assume_unique>(v_au.value)>(
                                    s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{-42});
                                REQUIRE(s1.empty());
                            }
                        } else {
                            s1 = s1_t{};
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{0});
                            REQUIRE(s1.size() == 1u);
                            REQUIRE(s1.begin()->first == pm_t{1, 2, 3});
                            REQUIRE(s1.begin()->second == 0);
                            s1.clear();

                            if (!v_au()) {
                                s1 = s1_t{};
                                s1.set_symbol_set(symbol_set{"x", "y", "z"});
                                detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                              static_cast<sat_check_compat_key>(v_cck.value),
                                                              static_cast<sat_check_table_size>(v_cts.value),
                                                              static_cast<sat_assume_unique>(v_au.value)>(
                                    s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{42});
                                detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                              static_cast<sat_check_compat_key>(v_cck.value),
                                                              static_cast<sat_check_table_size>(v_cts.value),
                                                              static_cast<sat_assume_unique>(v_au.value)>(
                                    s1, s1._get_s_table()[0], pm_t{1, 2, 3}, rat_t{-42});
                                REQUIRE(s1.size() == 1u);
                                REQUIRE(s1.begin()->first == pm_t{1, 2, 3});
                                REQUIRE(s1.begin()->second == 0);
                                s1.clear();
                            }
                        }

#if defined(MPPP_WITH_MPFR)
                        // Test coefficient move semantics with mppp::real.
#if defined(_MSC_VER)
                        // MSVC bug in VS2019. using vs. typedef should make no difference
                        typedef series<pm_t, real, void> s2_t;
#else
                        using s2_t = series<pm_t, real, void>;
#endif

                        s2_t s2;
                        real r{42};
                        s2.set_symbol_set(symbol_set{"x", "y", "z"});
                        detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                      static_cast<sat_check_compat_key>(v_cck.value),
                                                      static_cast<sat_check_table_size>(v_cts.value),
                                                      static_cast<sat_assume_unique>(v_au.value)>(
                            s2, s2._get_s_table()[0], pm_t{1, 2, 3}, std::move(r));
                        REQUIRE(s2.size() == 1u);
                        REQUIRE(s2.begin()->first == pm_t{1, 2, 3});
                        if (v_sign.value) {
                            REQUIRE(s2.begin()->second == 42);
                        } else {
                            REQUIRE(s2.begin()->second == -42);
                        }
                        REQUIRE(r._get_mpfr_t()->_mpfr_d == nullptr);

                        if (!v_au()) {
                            r = real{4, std::numeric_limits<int>::digits * 10};
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s2, s2._get_s_table()[0], pm_t{1, 2, 3}, std::move(r));
                            REQUIRE(s2.size() == 1u);
                            REQUIRE(s2.begin()->first == pm_t{1, 2, 3});
                            if (v_sign.value) {
                                REQUIRE(s2.begin()->second == 46);
                            } else {
                                REQUIRE(s2.begin()->second == -46);
                            }
                            REQUIRE(r.get_prec() == mppp::detail::real_deduce_precision(0));
                        }

                        if (!v_au()) {
                            // Test throwing + clearing within the insertion primitive.
                            r = real{"nan", 100};
                            s1 = s1_t{};
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{1, 2, 3}, 42);
                            detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                          static_cast<sat_check_compat_key>(v_cck.value),
                                                          static_cast<sat_check_table_size>(v_cts.value),
                                                          static_cast<sat_assume_unique>(v_au.value)>(
                                s1, s1._get_s_table()[0], pm_t{4, 5, 6}, -42);
                            REQUIRE(s1.size() == 2u);

                            OBAKE_REQUIRES_THROWS_CONTAINS(
                                (detail::series_add_term_table<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                               static_cast<sat_check_compat_key>(v_cck.value),
                                                               static_cast<sat_check_table_size>(v_cts.value),
                                                               static_cast<sat_assume_unique>(v_au.value)>(
                                    s1, s1._get_s_table()[0], pm_t{1, 2, 3}, r)),
                                std::exception, "Cannot convert a non-finite real to a rational");

                            REQUIRE(s1.empty());
                        }
#endif

                        // Tests with segmented table.
                        for (auto s_idx : {0u, 1u, 2u, 4u}) {
                            s1 = s1_t{};
                            s1.set_n_segments(s_idx);
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                rat_t{42});
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{4, 5, 6},
                                                                                                rat_t{43});
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{7, 8, 9},
                                                                                                rat_t{44});
                            REQUIRE(s1.size() == 3u);
                            if (v_sign.value) {
                                REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                    return p.second == 42 || p.second == 43 || p.second == 44;
                                }));
                            } else {
                                REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                    return p.second == -42 || p.second == -43 || p.second == -44;
                                }));
                            }
                        }

                        for (auto s_idx : {0u, 1u, 2u, 4u}) {
                            q = rat_t{42, 13};
                            s1 = s1_t{};
                            s1.set_n_segments(s_idx);
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3}, q);
                            q += 1;
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{4, 5, 6}, q);
                            q += 1;
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{7, 8, 9}, q);
                            REQUIRE(s1.size() == 3u);
                            if (v_sign.value) {
                                REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                    return p.second == rat_t{42, 13} || p.second == rat_t{42, 13} + 1
                                           || p.second == rat_t{42, 13} + 2;
                                }));
                            } else {
                                REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                    return p.second == rat_t{-42, 13} || p.second == rat_t{-42, 13} - 1
                                           || p.second == rat_t{-42, 13} - 2;
                                }));
                            }
                        }

                        for (auto s_idx : {0u, 1u, 2u, 4u}) {
                            s1 = s1_t{};
                            s1.set_n_segments(s_idx);
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3}, 42);
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{4, 5, 6}, 43);
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{7, 8, 9}, 44);
                            REQUIRE(s1.size() == 3u);
                            if (v_sign.value) {
                                REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                    return p.second == 42 || p.second == 43 || p.second == 44;
                                }));
                            } else {
                                REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                    return p.second == -42 || p.second == -43 || p.second == -44;
                                }));
                            }
                        }

                        for (auto s_idx : {0u, 1u, 2u, 4u}) {
                            s1 = s1_t{};
                            s1.set_n_segments(s_idx);
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3}, 42,
                                                                                                13);
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{4, 5, 6}, 43,
                                                                                                13);
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{7, 8, 9}, 44,
                                                                                                13);
                            REQUIRE(s1.size() == 3u);
                            if (v_sign.value) {
                                REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                    return p.second == rat_t{42, 13} || p.second == rat_t{43, 13}
                                           || p.second == rat_t{44, 13};
                                }));
                            } else {
                                REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                    return p.second == rat_t{-42, 13} || p.second == rat_t{-43, 13}
                                           || p.second == rat_t{-44, 13};
                                }));
                            }
                        }

                        if (!v_au()) {
                            for (auto s_idx : {0u, 1u, 2u, 4u}) {
                                s1 = s1_t{};
                                s1.set_n_segments(s_idx);
                                s1.set_symbol_set(symbol_set{"x", "y", "z"});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                    rat_t{42});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                    rat_t{43});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{7, 8, 9},
                                                                                                    rat_t{44});
                                REQUIRE(s1.size() == 2u);
                                if (v_sign.value) {
                                    REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                        return p.second == 85 || p.second == 44;
                                    }));
                                } else {
                                    REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                        return p.second == -85 || p.second == -44;
                                    }));
                                }
                            }

                            for (auto s_idx : {0u, 1u, 2u, 4u}) {
                                q = 1;
                                s1 = s1_t{};
                                s1.set_n_segments(s_idx);
                                s1.set_symbol_set(symbol_set{"x", "y", "z"});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                    rat_t{42});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                    q);
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{7, 8, 9},
                                                                                                    rat_t{44});
                                REQUIRE(s1.size() == 2u);
                                if (v_sign.value) {
                                    REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                        return p.second == 43 || p.second == 44;
                                    }));
                                } else {
                                    REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                        return p.second == -43 || p.second == -44;
                                    }));
                                }
                            }

                            for (auto s_idx : {0u, 1u, 2u, 4u}) {
                                s1 = s1_t{};
                                s1.set_n_segments(s_idx);
                                s1.set_symbol_set(symbol_set{"x", "y", "z"});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                    rat_t{42});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                    1);
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{7, 8, 9},
                                                                                                    rat_t{44});
                                REQUIRE(s1.size() == 2u);
                                if (v_sign.value) {
                                    REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                        return p.second == 43 || p.second == 44;
                                    }));
                                } else {
                                    REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                        return p.second == -43 || p.second == -44;
                                    }));
                                }
                            }

                            for (auto s_idx : {0u, 1u, 2u, 4u}) {
                                s1 = s1_t{};
                                s1.set_n_segments(s_idx);
                                s1.set_symbol_set(symbol_set{"x", "y", "z"});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                    rat_t{42});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                    42, 13);
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{7, 8, 9},
                                                                                                    rat_t{44});
                                REQUIRE(s1.size() == 2u);
                                if (v_sign.value) {
                                    REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                        return p.second == rat_t{588, 13} || p.second == 44;
                                    }));
                                } else {
                                    REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                        return p.second == -rat_t{588, 13} || p.second == -44;
                                    }));
                                }
                            }
                        }

                        // Check term annihilation or zero insertion.
                        if (v_cz()) {
                            for (auto s_idx : {0u, 1u, 2u, 4u}) {
                                s1 = s1_t{};
                                s1.set_n_segments(s_idx);
                                s1.set_symbol_set(symbol_set{"x", "y", "z"});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                    rat_t{});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{7, 8, 9},
                                                                                                    rat_t{44});
                                REQUIRE(s1.size() == 1u);
                                if (v_sign.value) {
                                    REQUIRE(s1.begin()->second == 44);
                                } else {
                                    REQUIRE(s1.begin()->second == -44);
                                }
                            }

                            if (!v_au()) {
                                for (auto s_idx : {0u, 1u, 2u, 4u}) {
                                    s1 = s1_t{};
                                    s1.set_n_segments(s_idx);
                                    s1.set_symbol_set(symbol_set{"x", "y", "z"});
                                    detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                            static_cast<sat_check_compat_key>(v_cck.value),
                                                            static_cast<sat_check_table_size>(v_cts.value),
                                                            static_cast<sat_assume_unique>(v_au.value)>(
                                        s1, pm_t{1, 2, 3}, rat_t{42});
                                    detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                            static_cast<sat_check_compat_key>(v_cck.value),
                                                            static_cast<sat_check_table_size>(v_cts.value),
                                                            static_cast<sat_assume_unique>(v_au.value)>(
                                        s1, pm_t{1, 2, 3}, rat_t{-42});
                                    detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                            static_cast<sat_check_compat_key>(v_cck.value),
                                                            static_cast<sat_check_table_size>(v_cts.value),
                                                            static_cast<sat_assume_unique>(v_au.value)>(
                                        s1, pm_t{7, 8, 9}, rat_t{44});
                                    REQUIRE(s1.size() == 1u);
                                    if (v_sign.value) {
                                        REQUIRE(s1.begin()->second == 44);
                                    } else {
                                        REQUIRE(s1.begin()->second == -44);
                                    }
                                }
                            }
                        } else {
                            for (auto s_idx : {0u, 1u, 2u, 4u}) {
                                s1 = s1_t{};
                                s1.set_n_segments(s_idx);
                                s1.set_symbol_set(symbol_set{"x", "y", "z"});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                    rat_t{});
                                detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                        static_cast<sat_check_compat_key>(v_cck.value),
                                                        static_cast<sat_check_table_size>(v_cts.value),
                                                        static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{7, 8, 9},
                                                                                                    rat_t{44});
                                REQUIRE(s1.size() == 2u);
                                if (v_sign.value) {
                                    REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                        return p.second == rat_t{44} || p.second == 0;
                                    }));
                                } else {
                                    REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                        return p.second == -rat_t{44} || p.second == 0;
                                    }));
                                }
                                s1.clear();
                            }

                            if (!v_au()) {
                                for (auto s_idx : {0u, 1u, 2u, 4u}) {
                                    s1 = s1_t{};
                                    s1.set_n_segments(s_idx);
                                    s1.set_symbol_set(symbol_set{"x", "y", "z"});
                                    detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                            static_cast<sat_check_compat_key>(v_cck.value),
                                                            static_cast<sat_check_table_size>(v_cts.value),
                                                            static_cast<sat_assume_unique>(v_au.value)>(
                                        s1, pm_t{1, 2, 3}, rat_t{42});
                                    detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                            static_cast<sat_check_compat_key>(v_cck.value),
                                                            static_cast<sat_check_table_size>(v_cts.value),
                                                            static_cast<sat_assume_unique>(v_au.value)>(
                                        s1, pm_t{1, 2, 3}, rat_t{-42});
                                    detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                            static_cast<sat_check_compat_key>(v_cck.value),
                                                            static_cast<sat_check_table_size>(v_cts.value),
                                                            static_cast<sat_assume_unique>(v_au.value)>(
                                        s1, pm_t{7, 8, 9}, rat_t{44});
                                    REQUIRE(s1.size() == 2u);
                                    if (v_sign.value) {
                                        REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                            return p.second == rat_t{44} || p.second == 0;
                                        }));
                                    } else {
                                        REQUIRE(std::all_of(s1.cbegin(), s1.cend(), [](const auto &p) {
                                            return p.second == -rat_t{44} || p.second == 0;
                                        }));
                                    }
                                    s1.clear();
                                }
                            }
                        }

#if defined(MPPP_WITH_MPFR)
                        // Test coefficient move semantics with mppp::real.

                        for (auto s_idx : {0u, 1u, 2u, 4u}) {
                            s2 = s2_t{};
                            s2.set_n_segments(s_idx);
                            r = real{42};
                            s2.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s2, pm_t{1, 2, 3},
                                                                                                std::move(r));
                            REQUIRE(s2.size() == 1u);
                            REQUIRE(s2.begin()->first == pm_t{1, 2, 3});
                            if (v_sign.value) {
                                REQUIRE(s2.begin()->second == 42);
                            } else {
                                REQUIRE(s2.begin()->second == -42);
                            }
                            REQUIRE(r._get_mpfr_t()->_mpfr_d == nullptr);
                        }

                        if (!v_au()) {
                            r = real{4, std::numeric_limits<int>::digits * 10};
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s2, pm_t{1, 2, 3},
                                                                                                std::move(r));
                            REQUIRE(s2.size() == 1u);
                            REQUIRE(s2.begin()->first == pm_t{1, 2, 3});
                            if (v_sign.value) {
                                REQUIRE(s2.begin()->second == 46);
                            } else {
                                REQUIRE(s2.begin()->second == -46);
                            }
                            REQUIRE(r.get_prec() == mppp::detail::real_deduce_precision(0));
                        }

                        if (!v_au()) {
                            // Test throwing + clearing within the insertion primitive.
                            r = real{"nan", 100};
                            s1 = s1_t{};
                            s1.set_symbol_set(symbol_set{"x", "y", "z"});
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3}, 42);
                            detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                    static_cast<sat_check_compat_key>(v_cck.value),
                                                    static_cast<sat_check_table_size>(v_cts.value),
                                                    static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{4, 5, 6}, -42);
                            REQUIRE(s1.size() == 2u);

                            OBAKE_REQUIRES_THROWS_CONTAINS(
                                (detail::series_add_term<v_sign.value, static_cast<sat_check_zero>(v_cz.value),
                                                         static_cast<sat_check_compat_key>(v_cck.value),
                                                         static_cast<sat_check_table_size>(v_cts.value),
                                                         static_cast<sat_assume_unique>(v_au.value)>(s1, pm_t{1, 2, 3},
                                                                                                     r)),
                                std::exception, "Cannot convert a non-finite real to a rational");

                            REQUIRE(s1.empty());
                        }
#endif
                    });
                });
            });
        });
    });

    // Check throw in case of incompatible key.
    s1_t s1;
    s1.set_symbol_set(symbol_set{});
    OBAKE_REQUIRES_THROWS_CONTAINS(
        (detail::series_add_term_table<true, sat_check_zero::on, sat_check_compat_key::on, sat_check_table_size::on,
                                       sat_assume_unique::off>(s1, s1._get_s_table()[0], pm_t(1), 1)),
        std::invalid_argument, "not compatible with the series' symbol set");

    for (auto s_idx : {0u, 1u, 2u, 4u}) {
        s1 = s1_t{};
        s1.set_n_segments(s_idx);
        s1.set_symbol_set(symbol_set{});
        OBAKE_REQUIRES_THROWS_CONTAINS(
            (detail::series_add_term<true, sat_check_zero::on, sat_check_compat_key::on, sat_check_table_size::on,
                                     sat_assume_unique::off>(s1, pm_t(1), 1)),
            std::invalid_argument, "not compatible with the series' symbol set");
    }
}

TEST_CASE("series_basic")
{
    using pm_t = packed_monomial<int>;
    using series_t = series<pm_t, rat_t, void>;

    // Default construction.
    series_t s;

    REQUIRE(s.empty());
    REQUIRE(s.size() == 0u);
    REQUIRE(s._get_s_table().size() == 1u);
    REQUIRE(s.get_symbol_set() == symbol_set{});
    s.set_n_segments(4u);
    REQUIRE(s.empty());
    REQUIRE(s.size() == 0u);
    REQUIRE(s._get_s_table().size() == 16u);
    s.set_symbol_set(symbol_set{"x", "y", "z"});
    REQUIRE(s.get_symbol_set() == symbol_set{"x", "y", "z"});

    // Copy construction.
    symbol_set ss{"x"};

    s = series_t{};
    s.set_symbol_set(ss);
    s.add_term(pm_t{2}, 4);
    REQUIRE(boost::contains(boost::lexical_cast<std::string>(s), "4*x**2"));
    REQUIRE(s.size() == 1u);

    {
        auto s_copy(s);
        REQUIRE(boost::contains(boost::lexical_cast<std::string>(s_copy), "4*x**2"));
        REQUIRE(s_copy.size() == 1u);
        REQUIRE(s_copy.get_symbol_set() == ss);
        REQUIRE(s_copy._get_s_table().size() == 1u);
    }

    // Try with a segmented series too.
    s = series_t{};
    s.set_symbol_set(ss);
    s.set_n_segments(3);
    s.add_term(pm_t{2}, 4);
    s.add_term(pm_t{0}, -1);
    s.add_term(pm_t{1}, -2);
    s.add_term(pm_t{3}, 9);
    REQUIRE(boost::contains(boost::lexical_cast<std::string>(s), "4*x**2"));
    REQUIRE(s.size() == 4u);

    {
        auto s_copy(s);
        REQUIRE(boost::contains(boost::lexical_cast<std::string>(s_copy), "4*x**2"));
        REQUIRE(s_copy.size() == 4u);
        REQUIRE(s_copy.get_symbol_set() == ss);
        REQUIRE(s_copy._get_s_table().size() == 8u);
    }

    // Move construction.
    s = series_t{};
    s.set_symbol_set(ss);
    s.add_term(pm_t{2}, 4);
    REQUIRE(boost::contains(boost::lexical_cast<std::string>(s), "4*x**2"));
    REQUIRE(s.size() == 1u);

    {
        auto s_move(std::move(s));
        REQUIRE(boost::contains(boost::lexical_cast<std::string>(s_move), "4*x**2"));
        REQUIRE(s_move.size() == 1u);
        REQUIRE(s_move.get_symbol_set() == ss);
        REQUIRE(s_move._get_s_table().size() == 1u);

        // Revive s.
        s = std::move(s_move);

        REQUIRE(boost::contains(boost::lexical_cast<std::string>(s), "4*x**2"));
        REQUIRE(s.size() == 1u);
    }

    // Try with a segmented series too.
    s = series_t{};
    s.set_symbol_set(ss);
    s.set_n_segments(3);
    s.add_term(pm_t{2}, 4);
    s.add_term(pm_t{0}, -1);
    s.add_term(pm_t{1}, -2);
    s.add_term(pm_t{3}, 9);
    REQUIRE(boost::contains(boost::lexical_cast<std::string>(s), "4*x**2"));
    REQUIRE(s.size() == 4u);

    {
        auto s_move(std::move(s));
        REQUIRE(boost::contains(boost::lexical_cast<std::string>(s_move), "4*x**2"));
        REQUIRE(s_move.size() == 4u);
        REQUIRE(s_move.get_symbol_set() == ss);
        REQUIRE(s_move._get_s_table().size() == 8u);

        // Revive s.
        s = std::move(s_move);

        REQUIRE(boost::contains(boost::lexical_cast<std::string>(s), "4*x**2"));
        REQUIRE(s.size() == 4u);
    }

    // Copy assignment.
    s = series_t{};
    s.set_symbol_set(ss);
    s.add_term(pm_t{2}, 4);
    s.add_term(pm_t{0}, -1);
    s.add_term(pm_t{1}, -2);
    s.add_term(pm_t{3}, 9);

    {
        series_t s2;
        s2 = s;
        REQUIRE(boost::contains(boost::lexical_cast<std::string>(s2), "4*x**2"));
        REQUIRE(s2.size() == 4u);
        REQUIRE(s2.get_symbol_set() == ss);
        REQUIRE(s2._get_s_table().size() == 1u);
    }

    // Try with a segmented series too.
    s = series_t{};
    s.set_symbol_set(ss);
    s.set_n_segments(3);
    s.add_term(pm_t{2}, 4);
    s.add_term(pm_t{0}, -1);
    s.add_term(pm_t{1}, -2);
    s.add_term(pm_t{3}, 9);

    {
        series_t s2;
        s2 = s;
        REQUIRE(boost::contains(boost::lexical_cast<std::string>(s2), "4*x**2"));
        REQUIRE(s2.size() == 4u);
        REQUIRE(s2.get_symbol_set() == ss);
        REQUIRE(s2._get_s_table().size() == 8u);
    }

    // Move assignment.
    s = series_t{};
    s.set_symbol_set(ss);
    s.add_term(pm_t{2}, 4);
    s.add_term(pm_t{0}, -1);
    s.add_term(pm_t{1}, -2);
    s.add_term(pm_t{3}, 9);

    {
        series_t s2;
        s2 = std::move(s);
        REQUIRE(boost::contains(boost::lexical_cast<std::string>(s2), "4*x**2"));
        REQUIRE(s2.size() == 4u);
        REQUIRE(s2.get_symbol_set() == ss);
        REQUIRE(s2._get_s_table().size() == 1u);
    }

    // Try with a segmented series too.
    s = series_t{};
    s.set_symbol_set(ss);
    s.set_n_segments(3);
    s.add_term(pm_t{2}, 4);
    s.add_term(pm_t{0}, -1);
    s.add_term(pm_t{1}, -2);
    s.add_term(pm_t{3}, 9);

    {
        series_t s2;
        s2 = std::move(s);
        REQUIRE(boost::contains(boost::lexical_cast<std::string>(s2), "4*x**2"));
        REQUIRE(s2.size() == 4u);
        REQUIRE(s2.get_symbol_set() == ss);
        REQUIRE(s2._get_s_table().size() == 8u);
    }
}

TEST_CASE("series_generic_ctor")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using s1_int_t = series<pm_t, int_t, void>;
    using s1_double_t = series<pm_t, double, void>;
    using s2_t = series<pm_t, s1_t, void>;

#if defined(MPPP_WITH_MPFR)
    using s1_real_t = series<pm_t, real, void>;
#endif

    REQUIRE(!std::is_constructible_v<s1_t, void>);

    // Constructability from non-series type.
    REQUIRE(std::is_constructible_v<s1_t, int>);
    REQUIRE(std::is_constructible_v<s1_t, int &>);
    REQUIRE(std::is_constructible_v<s1_t, const int &>);

    s1_t s1{5};
    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == 5);
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

    s1 = s1_t{0.};
    REQUIRE(s1.empty());
    REQUIRE(s1.get_symbol_set() == symbol_set{});

    s1 = s1_t{"3/4"};
    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == rat_t{3, 4});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

    s2_t s2{5};
    REQUIRE(s2.size() == 1u);
    REQUIRE(s2.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));
    s1 = s2.begin()->second;
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == 5);
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

    s2 = s2_t{0};
    REQUIRE(s2.empty());
    REQUIRE(s2.get_symbol_set() == symbol_set{});

    s2 = s2_t{"3/4"};
    REQUIRE(s2.size() == 1u);
    REQUIRE(s2.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));
    s1 = s2.begin()->second;
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == rat_t{3, 4});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

    // Constructability from lower rank series.
    REQUIRE(std::is_constructible_v<s2_t, s1_t>);
    REQUIRE(std::is_constructible_v<s2_t, s1_t &>);
    REQUIRE(std::is_constructible_v<s2_t, const s1_t &>);

    s2 = s2_t{s1_t{5}};
    REQUIRE(s2.size() == 1u);
    REQUIRE(s2.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));
    s1 = s2.begin()->second;
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == 5);
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

    s2 = s2_t{s1_t{0}};
    REQUIRE(s2.empty());
    REQUIRE(s2.get_symbol_set() == symbol_set{});

    s2 = s2_t{s1_t{"3/4"}};
    REQUIRE(s2.size() == 1u);
    REQUIRE(s2.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));
    s1 = s2.begin()->second;
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == rat_t{3, 4});
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

#if defined(MPPP_WITH_MPFR)

    // Verify that move construction moves.
    real r{42};
    s1_real_t s1r{std::move(r)};
    REQUIRE(r._get_mpfr_t()->_mpfr_d == nullptr);

#endif

    // Constructability from equal rank series.
    REQUIRE(std::is_constructible_v<s1_t, s1_int_t>);
    REQUIRE(std::is_constructible_v<s1_t, s1_int_t &>);
    REQUIRE(std::is_constructible_v<s1_t, const s1_int_t &>);
    REQUIRE(!std::is_constructible_v<s1_t, series<pm_t, rat_t, int>>);

    s1 = s1_t{s1_int_t{5}};
    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == 5);
    REQUIRE(s1.begin()->first == pm_t(symbol_set{}));

    s1_int_t s1_int{s1_t{"4/5"}};
    REQUIRE(s1_int.empty());

    for (auto s_idx : {0u, 1u, 2u, 3u, 4u}) {
        // Try with a more complex series and multiple segments.
        s1_int = s1_int_t{};
        s1_int.set_n_segments(s_idx);
        s1_int.set_symbol_set(symbol_set{"x", "y", "z"});
        s1_int.add_term(pm_t{1, 2, 3}, 1);
        s1_int.add_term(pm_t{-1, -2, -3}, -1);
        s1_int.add_term(pm_t{4, 5, 6}, 2);
        s1_int.add_term(pm_t{7, 8, 9}, -2);
        {
            s1_t s1a{s1_int};
            REQUIRE(s1a.size() == 4u);
            REQUIRE(s1a.get_s_size() == s_idx);
            for (const auto &p : s1a) {
                REQUIRE((abs(p.second) == 1 || abs(p.second) == 2));
            }

            s1_t s2a{std::move(s1_int)};
            REQUIRE(s2a.size() == 4u);
            REQUIRE(s2a.get_s_size() == s_idx);
            for (const auto &p : s2a) {
                REQUIRE((abs(p.second) == 1 || abs(p.second) == 2));
            }
        }

        // Verify construction of int series from double series
        // truncates and removes coefficients.
        s1_double_t s1_double;
        s1_double.set_n_segments(s_idx);
        s1_double.set_symbol_set(symbol_set{"x", "y", "z"});
        s1_double.add_term(pm_t{1, 2, 3}, .1);
        s1_double.add_term(pm_t{-1, -2, -3}, -.1);
        s1_double.add_term(pm_t{4, 5, 6}, .2);
        s1_double.add_term(pm_t{7, 8, 9}, -.2);
        REQUIRE(s1_int_t{s1_double}.empty());
        REQUIRE(s1_int_t{s1_double}.get_s_size() == s_idx);
    }

    // Construction from a series with higher rank.
    REQUIRE(std::is_constructible_v<s1_t, s2_t>);
    REQUIRE(std::is_constructible_v<s1_t, s2_t &>);
    REQUIRE(std::is_constructible_v<s1_t, const s2_t &>);

    REQUIRE(s1_t{s2_t{}}.empty());
    REQUIRE(s1_t{s2_t{0}}.empty());
    s1 = s1_t{s2_t{"4/5"}};
    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == rat_t{4, 5});

    s2 = s2_t{};
    s2.set_symbol_set(symbol_set{"x", "y", "z"});
    s2.add_term(pm_t{1, 2, 3}, 1);
    s2.add_term(pm_t{4, 5, 6}, 1);

    OBAKE_REQUIRES_THROWS_CONTAINS(s1_t{s2}, std::invalid_argument, "which does not consist of a single coefficient");
}

TEST_CASE("series_generic_assignment")
{
    // Just a couple of simple tests, this is implemented
    // on top of the generic ctor.
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using s1_int_t = series<pm_t, int_t, void>;
    using s2_t = series<pm_t, s1_t, void>;

    // Assignment from lower rank.
    s1_t s1;
    s1 = "3/4";
    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == rat_t{3, 4});

    s1 = 45;
    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == 45);

    s2_t s2;
    s2 = s1;
    REQUIRE(s2.size() == 1u);
    REQUIRE(s2.get_symbol_set() == symbol_set{});
    REQUIRE(s2.begin()->second.begin()->second == 45);

    // Assignment from equal rank.
    s1 = s1_int_t{-5};
    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == -5);

    // Assignment from higher rank.
    REQUIRE(&(s1 = s2_t{-1}) == &s1);
    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1.begin()->second == -1);

    REQUIRE(!std::is_assignable_v<s1_t, void>);
    REQUIRE(!std::is_assignable_v<s1_t, series<pm_t, rat_t, int>>);
}

TEST_CASE("series_swap")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    REQUIRE(std::is_nothrow_swappable_v<s1_t>);

    s1_t s0{"3/4"};

    s1_t s1;
    s1.set_n_segments(1);
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, 1);
    s1.add_term(pm_t{-1, -2, -3}, -1);
    s1.add_term(pm_t{4, 5, 6}, 2);
    s1.add_term(pm_t{7, 8, 9}, -2);

    using std::swap;
    swap(s0, s1);

    REQUIRE(s1.size() == 1u);
    REQUIRE(s1.get_symbol_set() == symbol_set{});
    REQUIRE(s1._get_s_table().size() == 1u);
    REQUIRE(s1.get_s_size() == 0u);

    REQUIRE(s0.size() == 4u);
    REQUIRE(s0.get_symbol_set() == symbol_set{"x", "y", "z"});
    REQUIRE(s0._get_s_table().size() == 2u);
    REQUIRE(s0.get_s_size() == 1u);
    for (const auto &p : s0) {
        REQUIRE((abs(p.second) == 1 || abs(p.second) == 2));
    }
}

TEST_CASE("series_is_single_cf")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    REQUIRE(s1_t{}.is_single_cf());
    REQUIRE(s1_t{42}.is_single_cf());

    s1_t s1;
    s1.set_n_segments(1);
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, 1);
    s1.add_term(pm_t{-1, -2, -3}, -1);
    s1.add_term(pm_t{4, 5, 6}, 2);
    s1.add_term(pm_t{7, 8, 9}, -2);
    REQUIRE(!s1.is_single_cf());
}

TEST_CASE("series_iterators")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using it_t = decltype(std::declval<s1_t &>().begin());
    using cit_t = decltype(std::declval<s1_t &>().cbegin());
    using const_it_t = decltype(std::declval<const s1_t &>().begin());

    REQUIRE(std::is_same_v<cit_t, const_it_t>);

    // Check value-inited iterators compare equal.
    REQUIRE(it_t{} == it_t{});
    REQUIRE(it_t{it_t{}} == it_t{});
    REQUIRE(cit_t{} == cit_t{});
    REQUIRE(cit_t{cit_t{}} == cit_t{});
    it_t it1{};
    cit_t cit1{};
    REQUIRE(it_t(it1) == it_t{});
    REQUIRE(cit_t(cit1) == cit_t{});

    // Require we can construct a const iterator from a mutable one.
    REQUIRE(cit_t(it_t{}) == cit_t{});
    // Require we can assign a mutable iterator to a const one.
    cit_t cit2{};
    cit2 = it1;
    REQUIRE(cit2 == it1);

    {
        // Swap tests.
        using std::swap;
        REQUIRE(std::is_nothrow_swappable_v<it_t>);
        REQUIRE(std::is_nothrow_swappable_v<cit_t>);

        s1_t s1{"4/5"};

        auto b = s1.begin();
        auto e = s1.end();
        swap(b, e);
        REQUIRE(b == s1.end());
        REQUIRE(e == s1.begin());

        auto cb = s1.cbegin();
        auto ce = s1.cend();
        swap(cb, ce);
        REQUIRE(cb == s1.cend());
        REQUIRE(ce == s1.cbegin());
    }

    {
        // Cross comparisons between const and mutable variants.
        s1_t s1{"4/5"};

        REQUIRE(s1.begin() == s1.cbegin());
        REQUIRE(s1.end() == s1.cend());
    }

    s1_t s1;
    REQUIRE(s1.begin() == s1.end());
    REQUIRE(s1.cbegin() == s1.cend());
    REQUIRE(s1.begin() == s1.cend());
    REQUIRE(s1.begin() == s1.cend());

    s1 = s1_t{"3/4"};
    REQUIRE(s1.begin() != s1.end());
    REQUIRE(s1.cbegin() != s1.cend());
    REQUIRE(s1.begin() != s1.cend());
    REQUIRE(s1.begin() != s1.cend());

    // A test with a segmented series.
    s1 = s1_t{};
    s1.set_n_segments(2);
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, 1);
    s1.add_term(pm_t{-1, -2, -3}, -1);
    s1.add_term(pm_t{4, 5, 6}, 2);
    s1.add_term(pm_t{7, 8, 9}, -2);

    REQUIRE(s1.begin() != s1.end());
    REQUIRE(s1.cbegin() != s1.cend());
    REQUIRE(s1.begin() != s1.cend());
    REQUIRE(s1.begin() != s1.cend());

    for (auto &p : s1) {
        REQUIRE((abs(p.second) == 1 || abs(p.second) == 2));
    }

    for (auto &p : static_cast<const s1_t &>(s1)) {
        REQUIRE((abs(p.second) == 1 || abs(p.second) == 2));
    }

    // Check the type yielded by dereferencing the iterators.
    REQUIRE(std::is_same_v<const std::pair<const pm_t, rat_t> &, decltype(*(static_cast<const s1_t &>(s1).begin()))>);
    REQUIRE(std::is_same_v<const std::pair<const pm_t, rat_t> &, decltype(*(s1.cbegin()))>);
    REQUIRE(std::is_same_v<std::pair<const pm_t, rat_t> &, decltype(*(s1.begin()))>);

    // Check that they are input iterators.
    REQUIRE(is_input_iterator_v<s1_t::iterator>);
    REQUIRE(is_input_iterator_v<s1_t::const_iterator>);
    REQUIRE(is_forward_iterator_v<s1_t::iterator>);
    REQUIRE(is_mutable_forward_iterator_v<s1_t::iterator>);
    REQUIRE(is_forward_iterator_v<s1_t::const_iterator>);
}
