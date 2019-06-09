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
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
// #include <cstddef>
// #include <bitset>
// #include <limits>
// #include <iostream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include <mp++/config.hpp>
#include <mp++/rational.hpp>

#if defined(MPPP_WITH_MPFR)
#include <mp++/real.hpp>
#endif

#include <piranha/config.hpp>
#include <piranha/detail/tuple_for_each.hpp>
#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>
// #include <piranha/hash.hpp>
// #include <piranha/math/is_zero.hpp>
// #include <piranha/math/negate.hpp>
// #include <piranha/math/pow.hpp>

using namespace piranha;

using rat_t = mppp::rational<1>;

// template <typename T, typename U>
// using series_add_t = decltype(series_add(std::declval<T>(), std::declval<U>()));

TEST_CASE("cf_key_concepts")
{
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

#if defined(PIRANHA_HAVE_CONCEPTS)
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

#if defined(PIRANHA_HAVE_CONCEPTS)
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

    using Catch::Matchers::Contains;

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

                            if (!decltype(v_au)::value) {
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
                    });
                });
            });
        });
    });

    // Check throw in case of incompatible key.
    s1_t s1;
    s1.set_symbol_set(symbol_set{});
    REQUIRE_THROWS_WITH(
        (detail::series_add_term_table<true, sat_check_zero::on, sat_check_compat_key::on, sat_check_table_size::on,
                                       sat_assume_unique::off>(s1, s1._get_s_table()[0], pm_t(1), 1)),
        Contains("not compatible with the series' symbol set"));
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

#if 0

TEST_CASE("pow_test")
{
    using rat_t = mppp::rational<1>;

    using pm_t = packed_monomial<int>;
    using series_t = series<pm_t, double, void>;
    using series_t_rat = series<pm_t, rat_t, void>;

    REQUIRE(series_rank<void> == 0u);
    REQUIRE(series_rank<series_t> == 1u);
    REQUIRE(series_rank<series_t &> == 0u);

    REQUIRE(!is_detected_v<series_add_t, int, int>);

    REQUIRE(detail::series_add_algorithm<int &, int &> == 0);
    REQUIRE(detail::series_add_algorithm<int &, series_t &> == 1);
    REQUIRE(detail::series_add_algorithm<series_t &, int &> == 2);

    series_t s, s2(4);
    s.set_nsegments(4);
    REQUIRE(s2.is_single_cf());
    REQUIRE(s.is_single_cf());
    std::cout << s2 + 3.5 << '\n';

    series_t_rat sa(s2 + 3.5);
    std::cout << sa << '\n';
    std::cout << sa - 15.63 << '\n';
    REQUIRE(is_zero(sa - sa));
    REQUIRE(is_cf_v<series_t_rat>);

    std::cout << s << '\n';
    REQUIRE(s.empty());
    REQUIRE(s.begin() == s.end());
    REQUIRE(s.cbegin() == s.cend());

    REQUIRE(s.cbegin() == s.begin());

    series_t::const_iterator it0(s.begin());
    // series_t::iterator it1(s.cbegin());

    REQUIRE(std::is_nothrow_swappable_v<series_t>);
    REQUIRE(std::is_nothrow_swappable_v<series_t::const_iterator>);
    REQUIRE(std::is_nothrow_swappable_v<series_t::iterator>);

    constexpr auto width = std::numeric_limits<std::size_t>::digits;

    std::cout << std::bitset<width>(detail::series_key_hasher{}(pm_t{1, 2, 3})) << " vs "
              << std::bitset<width>(hash(pm_t{1, 2, 3})) << '\n';
    std::cout << std::bitset<width>(detail::series_key_hasher{}(pm_t{4, 5, 6})) << " vs "
              << std::bitset<width>(hash(pm_t{4, 5, 6})) << '\n';

    s.set_symbol_set({"x", "y", "z"});
    s.add_term(pm_t{0, 1, 2}, -1.);
    s.add_term(pm_t{2, 4, 5}, -2.);
    s.add_term(pm_t{2, 4, 5}, -5.);
    s.add_term(pm_t{0, 0, 0}, -1.);
    s.add_term(pm_t{0, 0, 0}, 1.);
    s.add_term(pm_t{0, 0, 0}, -1.);

    s.add_term<false>(pm_t{2, 0, -1}, -1.3);
    s.add_term(pm_t{2, 1, -1}, -1.);
    s.add_term<false>(pm_t{2, 1, -1}, -1.);

    REQUIRE(!s.is_single_cf());

    std::cout << s << '\n';

    s = rat_t{1, 2};
    std::cout << s << '\n';

    series_t_rat farp{"3/4"};
    std::cout << farp << '\n';
    std::cout << static_cast<double>(farp) << '\n';

    REQUIRE(!is_series_constructible_v<void, void, void, void>);
    REQUIRE(!is_series_convertible_v<void, void>);

    std::cout << -+-+negate(s + s) << '\n';

    REQUIRE(is_negatable_v<series_t &>);
    REQUIRE(is_negatable_v<series_t &&>);
    REQUIRE(!is_negatable_v<const series_t &>);
    REQUIRE(!is_negatable_v<const series_t &&>);
}

#endif
