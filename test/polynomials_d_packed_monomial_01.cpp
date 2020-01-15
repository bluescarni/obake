// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <initializer_list>
#include <list>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/hash.hpp>
#include <obake/k_packing.hpp>
#include <obake/key/key_degree.hpp>
#include <obake/key/key_merge_symbols.hpp>
#include <obake/key/key_p_degree.hpp>
#include <obake/polynomials/d_packed_monomial.hpp>
#include <obake/polynomials/monomial_homomorphic_hash.hpp>
#include <obake/polynomials/monomial_mul.hpp>
#include <obake/polynomials/monomial_pow.hpp>
#include <obake/polynomials/monomial_range_overflow_check.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

using int_types = std::tuple<int, unsigned, long, unsigned long, long long, unsigned long long
// NOTE: clang + ubsan fail to compile with 128bit integers in this test.
#if defined(OBAKE_HAVE_GCC_INT128) && !defined(OBAKE_TEST_CLANG_UBSAN)
                             ,
                             __int128_t, __uint128_t
#endif
                             >;

// The bit widths over which we will be testing for type T.
template <typename T>
using bits_widths = std::tuple<std::integral_constant<unsigned, 3>, std::integral_constant<unsigned, 6>,
#if !defined(_MSC_VER) || defined(__clang__)
                               std::integral_constant<unsigned, detail::limits_digits<T> / 2>,
#endif
                               std::integral_constant<unsigned, detail::limits_digits<T>>>;

std::mt19937 rng;

TEST_CASE("homomorphic_hash_test")
{
    obake_test::disable_slow_stack_traces();

    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto b) {
            constexpr auto bw = decltype(b)::value;
            using pm_t = d_packed_monomial<int_t, bw>;
            using c_t = typename pm_t::container_t;

            REQUIRE(is_homomorphically_hashable_monomial_v<pm_t>);
            REQUIRE(!is_homomorphically_hashable_monomial_v<pm_t &>);
            REQUIRE(!is_homomorphically_hashable_monomial_v<pm_t &&>);
            REQUIRE(!is_homomorphically_hashable_monomial_v<const pm_t &>);

            // Random testing.
            if constexpr (bw >= 6u
#if defined(OBAKE_HAVE_GCC_INT128)
                          && !std::is_same_v<__int128_t, int_t> && !std::is_same_v<__uint128_t, int_t>
#endif
            ) {
                using idist_t = std::uniform_int_distribution<detail::make_dependent_t<int_t, decltype(b)>>;
                using param_t = typename idist_t::param_type;
                idist_t dist;

                c_t tmp1, tmp2, tmp3;

                for (auto i = 0u; i < 100u; ++i) {
                    tmp1.resize(i);
                    tmp2.resize(i);
                    tmp3.resize(i);

                    for (auto j = 0u; j < i; ++j) {
                        if constexpr (is_signed_v<int_t>) {
                            tmp1[j] = dist(rng, param_t{-10, 10});
                            tmp2[j] = dist(rng, param_t{-10, 10});
                        } else {
                            tmp1[j] = dist(rng, param_t{0, 20});
                            tmp2[j] = dist(rng, param_t{0, 20});
                        }
                        tmp3[j] = tmp1[j] + tmp2[j];
                    }

                    // Construct the monomials.
                    pm_t pm1(tmp1.data(), i), pm2(tmp2.data(), i), pm3(tmp3.data(), i);

                    // Do the hashing, check the sum.
                    REQUIRE(hash(pm1) + hash(pm2) == hash(pm3));
                }
            }
        });
    });
}

TEST_CASE("key_merge_symbols_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto b) {
            constexpr auto bw = decltype(b)::value;
            using pm_t = d_packed_monomial<int_t, bw>;

            REQUIRE(is_symbols_mergeable_key_v<pm_t>);
            REQUIRE(is_symbols_mergeable_key_v<pm_t &>);
            REQUIRE(is_symbols_mergeable_key_v<pm_t &&>);
            REQUIRE(is_symbols_mergeable_key_v<const pm_t &>);

            if constexpr (bw >= 6u) {
                REQUIRE(key_merge_symbols(pm_t{}, symbol_idx_map<symbol_set>{}, symbol_set{}) == pm_t{});
                REQUIRE(key_merge_symbols(pm_t{}, symbol_idx_map<symbol_set>{{0, {"x"}}}, symbol_set{}) == pm_t{0});
                REQUIRE(key_merge_symbols(pm_t{1}, symbol_idx_map<symbol_set>{}, symbol_set{"x"}) == pm_t{1});
                REQUIRE(key_merge_symbols(pm_t{1}, symbol_idx_map<symbol_set>{{0, {"y"}}}, symbol_set{"x"})
                        == pm_t{0, 1});
                REQUIRE(key_merge_symbols(pm_t{1}, symbol_idx_map<symbol_set>{{1, {"y"}}}, symbol_set{"x"})
                        == pm_t{1, 0});
                REQUIRE(key_merge_symbols(pm_t{1, 2, 3},
                                          symbol_idx_map<symbol_set>{{0, {"a", "b"}}, {1, {"c"}}, {3, {"d", "e"}}},
                                          symbol_set{"x", "y", "z"})
                        == pm_t{0, 0, 1, 0, 2, 3, 0, 0});
                REQUIRE(key_merge_symbols(pm_t{1, 2, 3}, symbol_idx_map<symbol_set>{{3, {"d", "e"}}},
                                          symbol_set{"x", "y", "z"})
                        == pm_t{1, 2, 3, 0, 0});
                REQUIRE(key_merge_symbols(pm_t{1, 2, 3}, symbol_idx_map<symbol_set>{{0, {"d", "e"}}},
                                          symbol_set{"x", "y", "z"})
                        == pm_t{0, 0, 1, 2, 3});
                REQUIRE(key_merge_symbols(pm_t{1, 2, 3}, symbol_idx_map<symbol_set>{{1, {"d", "e"}}},
                                          symbol_set{"x", "y", "z"})
                        == pm_t{1, 0, 0, 2, 3});

                if constexpr (is_signed_v<int_t>) {
                    REQUIRE(key_merge_symbols(pm_t{-1}, symbol_idx_map<symbol_set>{}, symbol_set{"x"}) == pm_t{-1});
                    REQUIRE(key_merge_symbols(pm_t{-1}, symbol_idx_map<symbol_set>{{0, {"y"}}}, symbol_set{"x"})
                            == pm_t{0, -1});
                    REQUIRE(key_merge_symbols(pm_t{-1}, symbol_idx_map<symbol_set>{{1, {"y"}}}, symbol_set{"x"})
                            == pm_t{-1, 0});
                    REQUIRE(key_merge_symbols(pm_t{-1, -2, -3},
                                              symbol_idx_map<symbol_set>{{0, {"a", "b"}}, {1, {"c"}}, {3, {"d", "e"}}},
                                              symbol_set{"x", "y", "z"})
                            == pm_t{0, 0, -1, 0, -2, -3, 0, 0});
                    REQUIRE(key_merge_symbols(pm_t{-1, -2, -3}, symbol_idx_map<symbol_set>{{3, {"d", "e"}}},
                                              symbol_set{"x", "y", "z"})
                            == pm_t{-1, -2, -3, 0, 0});
                    REQUIRE(key_merge_symbols(pm_t{-1, -2, -3}, symbol_idx_map<symbol_set>{{0, {"d", "e"}}},
                                              symbol_set{"x", "y", "z"})
                            == pm_t{0, 0, -1, -2, -3});
                    REQUIRE(key_merge_symbols(pm_t{-1, -2, -3}, symbol_idx_map<symbol_set>{{1, {"d", "e"}}},
                                              symbol_set{"x", "y", "z"})
                            == pm_t{-1, 0, 0, -2, -3});
                }
            }
        });
    });
}

TEST_CASE("monomial_mul_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto bs) {
            constexpr auto bw = decltype(bs)::value;
            using pm_t = d_packed_monomial<int_t, bw>;

            REQUIRE(is_multipliable_monomial_v<pm_t &, const pm_t &, const pm_t &>);
            REQUIRE(is_multipliable_monomial_v<pm_t &, pm_t &, pm_t &>);
            REQUIRE(is_multipliable_monomial_v<pm_t &, pm_t &&, pm_t &&>);
            REQUIRE(!is_multipliable_monomial_v<const pm_t &, const pm_t &, const pm_t &>);
            REQUIRE(!is_multipliable_monomial_v<pm_t &&, const pm_t &, const pm_t &>);

            pm_t a, b, c;
            monomial_mul(a, b, c, symbol_set{});
            REQUIRE(a == pm_t{});

            b = pm_t{0, 1, 0};
            c = pm_t{1, 1, 0};
            a = pm_t{1, 1, 1};
            monomial_mul(a, b, c, symbol_set{"x", "y", "z"});
            REQUIRE(a == pm_t{1, 2, 0});

            if constexpr (bw >= 6u) {
                b = pm_t{1, 2, 3};
                c = pm_t{4, 5, 6};
                a = pm_t{0, 1, 0};
                monomial_mul(a, b, c, symbol_set{"x", "y", "z"});
                REQUIRE(a == pm_t{5, 7, 9});
            }
        });
    });
}

TEST_CASE("range_overflow_check_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto bs) {
            constexpr auto bw = decltype(bs)::value;
            using pm_t = d_packed_monomial<int_t, bw>;

            // Check the type trait.
            REQUIRE(are_overflow_testable_monomial_ranges_v<std::vector<pm_t>, std::vector<pm_t>>);
            REQUIRE(are_overflow_testable_monomial_ranges_v<std::vector<pm_t>, std::list<pm_t>>);
            REQUIRE(are_overflow_testable_monomial_ranges_v<std::list<pm_t>, std::vector<pm_t>>);

            REQUIRE(!are_overflow_testable_monomial_ranges_v<std::vector<pm_t>, void>);
            REQUIRE(!are_overflow_testable_monomial_ranges_v<void, std::vector<pm_t>>);

#if defined(OBAKE_HAVE_CONCEPTS)
            REQUIRE(OverflowTestableMonomialRanges<std::vector<pm_t>, std::vector<pm_t>>);
            REQUIRE(OverflowTestableMonomialRanges<std::vector<pm_t>, std::list<pm_t>>);
            REQUIRE(OverflowTestableMonomialRanges<std::list<pm_t>, std::vector<pm_t>>);

            REQUIRE(!OverflowTestableMonomialRanges<std::vector<pm_t>, void>);
            REQUIRE(!OverflowTestableMonomialRanges<void, std::vector<pm_t>>);
#endif

            std::vector<pm_t> v1, v2;
            symbol_set ss;

            // Empty symbol set.
            REQUIRE(monomial_range_overflow_check(v1, v2, ss));

            // Both empty ranges.
            ss = symbol_set{"x", "y", "z"};
            REQUIRE(monomial_range_overflow_check(v1, v2, ss));

            // Empty second range.
            v1.emplace_back(pm_t{1, 0, 1});
            REQUIRE(monomial_range_overflow_check(v1, v2, ss));

            if constexpr (bw >= 6u) {
                if constexpr (bw == static_cast<unsigned>(detail::limits_digits<int_t>)) {
                    // Special-casing for maximal packing.
                    v1.clear();
                    v2.clear();
                    ss = symbol_set{"x", "y", "z"};

                    v1.emplace_back(pm_t{1, 2, 3});
                    v2.emplace_back(pm_t{2, 4, 5});
                    REQUIRE(monomial_range_overflow_check(v1, v2, ss));
                    v1.emplace_back(pm_t{4, 6, 8});
                    REQUIRE(monomial_range_overflow_check(v1, v2, ss));
                    v1.emplace_back(pm_t{2, 7, 16});
                    v1.emplace_back(pm_t{2, 4, 9});
                    v1.emplace_back(pm_t{0, 0, 2});
                    v2.emplace_back(pm_t{2, 0, 5});
                    v2.emplace_back(pm_t{1, 0, 64});
                    v2.emplace_back(pm_t{0, 0, 0});
                    REQUIRE(monomial_range_overflow_check(v1, v2, ss));

                    if constexpr (is_signed_v<int_t>) {
                        v1.emplace_back(pm_t{-2, -4, 6});
                        v1.emplace_back(pm_t{2, 5, -9});
                        v1.emplace_back(pm_t{0, 4, -5});
                        v2.emplace_back(pm_t{-2, 4, 5});
                        v2.emplace_back(pm_t{1, -67, -6});
                        v2.emplace_back(pm_t{0, 1, 1});
                        REQUIRE(monomial_range_overflow_check(v1, v2, ss));
                    }

                    // Overflow check.
                    if constexpr (is_signed_v<int_t>) {
                        v1.emplace_back(pm_t{int_t(1), int_t(2), detail::limits_min<int_t>});
                        REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                        v1.pop_back();

                        v1.emplace_back(pm_t{detail::limits_max<int_t>, int_t(6), int_t(7)});
                        REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                        v1.pop_back();
                    } else {
                        v1.emplace_back(pm_t{detail::limits_max<int_t>, int_t(1), int_t(2)});
                        REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                    }
                } else {
                    // Simple tests.
                    v2.emplace_back(pm_t{1, 2, 3});
                    REQUIRE(monomial_range_overflow_check(v1, v2, ss));
                    v1.emplace_back(pm_t{4, 5, 6});
                    REQUIRE(monomial_range_overflow_check(v1, v2, ss));
                    v1.emplace_back(pm_t{2, 1, 3});
                    v1.emplace_back(pm_t{2, 1, 7});
                    v1.emplace_back(pm_t{0, 1, 0});
                    v2.emplace_back(pm_t{2, 0, 3});
                    v2.emplace_back(pm_t{1, 1, 1});
                    v2.emplace_back(pm_t{0, 4, 1});
                    REQUIRE(monomial_range_overflow_check(v1, v2, ss));

                    if constexpr (is_signed_v<int_t>) {
                        // Negatives as well.
                        v1.emplace_back(pm_t{-2, 1, 3});
                        v1.emplace_back(pm_t{2, 1, -7});
                        v1.emplace_back(pm_t{0, -1, 0});
                        v2.emplace_back(pm_t{-2, 0, 3});
                        v2.emplace_back(pm_t{1, -1, -1});
                        v2.emplace_back(pm_t{0, -4, 1});
                        REQUIRE(monomial_range_overflow_check(v1, v2, ss));
                    }

                    // Check overflow now.
                    {
                        // Get the limits of the component at index 1.
                        const auto &lims = detail::k_packing_get_climits<int_t>(bw, 1);
                        if constexpr (is_signed_v<int_t>) {
                            v1.emplace_back(pm_t{int_t(0), lims[0], int_t(4)});
                            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                            v1.pop_back();

                            v1.emplace_back(pm_t{int_t(0), lims[1], int_t(4)});
                            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                            v1.pop_back();
                        } else {
                            v1.emplace_back(pm_t{int_t(0), lims, int_t(4)});
                            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                        }
                    }
                    {
                        // Get the limits of the component at index 0.
                        const auto &lims = detail::k_packing_get_climits<int_t>(bw, 0);
                        if constexpr (is_signed_v<int_t>) {
                            v1.emplace_back(pm_t{lims[0], int_t(0), int_t(4)});
                            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                            v1.pop_back();

                            v1.emplace_back(pm_t{lims[1], int_t(0), int_t(4)});
                            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                            v1.pop_back();
                        } else {
                            v1.emplace_back(pm_t{lims, int_t(0), int_t(4)});
                            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                        }
                    }
                }
            }
        });
    });
}

TEST_CASE("degree_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto bs) {
            constexpr auto bw = decltype(bs)::value;
            using pm_t = d_packed_monomial<int_t, bw>;

            if constexpr (bw >= 6u) {
                REQUIRE(key_degree(pm_t{}, symbol_set{}) == int_t(0));
                REQUIRE(key_degree(pm_t{1}, symbol_set{"x"}) == int_t(1));
                REQUIRE(key_degree(pm_t{4}, symbol_set{"x"}) == int_t(4));

                if constexpr (is_signed_v<int_t>) {
                    REQUIRE(key_degree(pm_t{-1}, symbol_set{"x"}) == int_t(-1));
                    REQUIRE(key_degree(pm_t{-4}, symbol_set{"x"}) == int_t(-4));
                }

                REQUIRE(key_degree(pm_t{1, 2}, symbol_set{"x", "y"}) == int_t(3));
                REQUIRE(key_degree(pm_t{2, 3}, symbol_set{"x", "y"}) == int_t(5));

                if constexpr (is_signed_v<int_t>) {
                    REQUIRE(key_degree(pm_t{-1, 2}, symbol_set{"x", "y"}) == int_t(1));
                    REQUIRE(key_degree(pm_t{-2, 5}, symbol_set{"x", "y"}) == int_t(3));
                }
            }

            REQUIRE(is_key_with_degree_v<pm_t>);
            REQUIRE(is_key_with_degree_v<pm_t &>);
            REQUIRE(is_key_with_degree_v<const pm_t &>);
            REQUIRE(is_key_with_degree_v<pm_t &&>);
        });
    });
}

TEST_CASE("p_degree_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto bs) {
            constexpr auto bw = decltype(bs)::value;
            using pm_t = d_packed_monomial<int_t, bw>;

            if constexpr (bw >= 6u) {
                REQUIRE(key_p_degree(pm_t{}, symbol_idx_set{}, symbol_set{}) == int_t(0));
                REQUIRE(key_p_degree(pm_t{1}, symbol_idx_set{0}, symbol_set{"x"}) == int_t(1));
                REQUIRE(key_p_degree(pm_t{1}, symbol_idx_set{}, symbol_set{"x"}) == int_t(0));
                REQUIRE(key_p_degree(pm_t{12}, symbol_idx_set{0}, symbol_set{"x"}) == int_t(12));
                REQUIRE(key_p_degree(pm_t{12}, symbol_idx_set{}, symbol_set{"x"}) == int_t(0));

                if constexpr (is_signed_v<int_t>) {
                    REQUIRE(key_p_degree(pm_t{-1}, symbol_idx_set{0}, symbol_set{"x"}) == int_t(-1));
                    REQUIRE(key_p_degree(pm_t{-1}, symbol_idx_set{}, symbol_set{"x"}) == int_t(0));
                    REQUIRE(key_p_degree(pm_t{-12}, symbol_idx_set{0}, symbol_set{"x"}) == int_t(-12));
                    REQUIRE(key_p_degree(pm_t{-12}, symbol_idx_set{}, symbol_set{"x"}) == int_t(0));
                }

                REQUIRE(key_p_degree(pm_t{1, 2}, symbol_idx_set{0, 1}, symbol_set{"x", "y"}) == int_t(3));
                REQUIRE(key_p_degree(pm_t{1, 2}, symbol_idx_set{0}, symbol_set{"x", "y"}) == int_t(1));
                REQUIRE(key_p_degree(pm_t{1, 2}, symbol_idx_set{1}, symbol_set{"x", "y"}) == int_t(2));
                REQUIRE(key_p_degree(pm_t{1, 2}, symbol_idx_set{}, symbol_set{"x", "y"}) == int_t(0));
                REQUIRE(key_p_degree(pm_t{12, 3}, symbol_idx_set{0, 1}, symbol_set{"x", "y"}) == int_t(15));
                REQUIRE(key_p_degree(pm_t{12, 3}, symbol_idx_set{0}, symbol_set{"x", "y"}) == int_t(12));
                REQUIRE(key_p_degree(pm_t{12, 3}, symbol_idx_set{1}, symbol_set{"x", "y"}) == int_t(3));
                REQUIRE(key_p_degree(pm_t{12, 3}, symbol_idx_set{}, symbol_set{"x", "y"}) == int_t(0));

                REQUIRE(key_p_degree(pm_t{1, 2, 3}, symbol_idx_set{0, 1, 2}, symbol_set{"x", "y", "z"}) == int_t(6));
                REQUIRE(key_p_degree(pm_t{1, 2, 3}, symbol_idx_set{}, symbol_set{"x", "y", "z"}) == int_t(0));
                REQUIRE(key_p_degree(pm_t{1, 2, 3}, symbol_idx_set{0}, symbol_set{"x", "y", "z"}) == int_t(1));
                REQUIRE(key_p_degree(pm_t{1, 2, 3}, symbol_idx_set{1}, symbol_set{"x", "y", "z"}) == int_t(2));
                REQUIRE(key_p_degree(pm_t{1, 2, 3}, symbol_idx_set{2}, symbol_set{"x", "y", "z"}) == int_t(3));
                REQUIRE(key_p_degree(pm_t{1, 2, 3}, symbol_idx_set{0, 1}, symbol_set{"x", "y", "z"}) == int_t(3));
                REQUIRE(key_p_degree(pm_t{1, 2, 3}, symbol_idx_set{1, 2}, symbol_set{"x", "y", "z"}) == int_t(5));
                REQUIRE(key_p_degree(pm_t{1, 2, 3}, symbol_idx_set{0, 2}, symbol_set{"x", "y", "z"}) == int_t(4));

                if constexpr (is_signed_v<int_t>) {
                    REQUIRE(key_p_degree(pm_t{-1, 2}, symbol_idx_set{0, 1}, symbol_set{"x", "y"}) == int_t(1));
                    REQUIRE(key_p_degree(pm_t{-1, 2}, symbol_idx_set{0}, symbol_set{"x", "y"}) == int_t(-1));
                    REQUIRE(key_p_degree(pm_t{-1, 2}, symbol_idx_set{1}, symbol_set{"x", "y"}) == int_t(2));
                    REQUIRE(key_p_degree(pm_t{-1, 2}, symbol_idx_set{}, symbol_set{"x", "y"}) == int_t(0));
                    REQUIRE(key_p_degree(pm_t{-12, 5}, symbol_idx_set{0, 1}, symbol_set{"x", "y"}) == int_t(-7));
                    REQUIRE(key_p_degree(pm_t{-12, 5}, symbol_idx_set{0}, symbol_set{"x", "y"}) == int_t(-12));
                    REQUIRE(key_p_degree(pm_t{-12, 5}, symbol_idx_set{1}, symbol_set{"x", "y"}) == int_t(5));
                    REQUIRE(key_p_degree(pm_t{-12, 5}, symbol_idx_set{}, symbol_set{"x", "y"}) == int_t(0));
                }
            }

            REQUIRE(is_key_with_p_degree_v<pm_t>);
            REQUIRE(is_key_with_p_degree_v<pm_t &>);
            REQUIRE(is_key_with_p_degree_v<const pm_t &>);
            REQUIRE(is_key_with_p_degree_v<pm_t &&>);
        });
    });
}

TEST_CASE("monomial_pow_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto bs) {
            constexpr auto bw = decltype(bs)::value;
            using pm_t = d_packed_monomial<int_t, bw>;

            REQUIRE(!is_exponentiable_monomial_v<pm_t, void>);
            REQUIRE(!is_exponentiable_monomial_v<void, pm_t>);

            REQUIRE(is_exponentiable_monomial_v<pm_t, int>);
            REQUIRE(is_exponentiable_monomial_v<const pm_t, int &>);
            REQUIRE(is_exponentiable_monomial_v<const pm_t &, const int &>);
            REQUIRE(is_exponentiable_monomial_v<pm_t &, const int>);
            REQUIRE(!is_exponentiable_monomial_v<pm_t &, std::string>);

            if constexpr (bw >= 6u) {
                REQUIRE(monomial_pow(pm_t{}, 0, symbol_set{}) == pm_t{});
                REQUIRE(monomial_pow(pm_t{1}, 0, symbol_set{"x"}) == pm_t{0});
                REQUIRE(monomial_pow(pm_t{2}, 0, symbol_set{"x"}) == pm_t{0});
                REQUIRE(monomial_pow(pm_t{2}, mppp::integer<1>{1}, symbol_set{"x"}) == pm_t{2});
                REQUIRE(monomial_pow(pm_t{1, 2, 3}, 0, symbol_set{"x", "y", "z"}) == pm_t{0, 0, 0});
                REQUIRE(monomial_pow(pm_t{1, 2, 3}, 1, symbol_set{"x", "y", "z"}) == pm_t{1, 2, 3});
                REQUIRE(monomial_pow(pm_t{1, 2, 3}, 2, symbol_set{"x", "y", "z"}) == pm_t{2, 4, 6});
                REQUIRE(monomial_pow(pm_t{1, 2, 3}, mppp::integer<2>{4}, symbol_set{"x", "y", "z"}) == pm_t{4, 8, 12});
                REQUIRE(monomial_pow(pm_t{1, 2, 3}, mppp::rational<1>{4}, symbol_set{"x", "y", "z"}) == pm_t{4, 8, 12});
                OBAKE_REQUIRES_THROWS_CONTAINS(
                    monomial_pow(pm_t{1, 2, 3}, mppp::rational<1>{4, 3}, symbol_set{"x", "y", "z"}),
                    std::invalid_argument,
                    "Invalid exponent for monomial exponentiation: the exponent (4/3) cannot be converted into an "
                    "integral "
                    "value");

                // Check overflows, both in the single exponent exponentiation and in the coding limits.
                OBAKE_REQUIRES_THROWS_CONTAINS(monomial_pow(pm_t{detail::limits_max<int_t>}, 2, symbol_set{"x"}),
                                               std::overflow_error, "");

                if constexpr (is_signed_v<int_t>) {
                    REQUIRE(monomial_pow(pm_t{-1}, 0, symbol_set{"x"}) == pm_t{0});
                    REQUIRE(monomial_pow(pm_t{-2}, 0, symbol_set{"x"}) == pm_t{0});
                    REQUIRE(monomial_pow(pm_t{-2}, 1, symbol_set{"x"}) == pm_t{-2});
                    REQUIRE(monomial_pow(pm_t{-1, 2, -3}, 0, symbol_set{"x", "y", "z"}) == pm_t{0, 0, 0});
                    REQUIRE(monomial_pow(pm_t{-1, 2, -3}, mppp::integer<1>{1}, symbol_set{"x", "y", "z"})
                            == pm_t{-1, 2, -3});
                    REQUIRE(monomial_pow(pm_t{1, -2, 3}, 2, symbol_set{"x", "y", "z"}) == pm_t{2, -4, 6});
                    REQUIRE(monomial_pow(pm_t{1, -2, 3}, mppp::integer<2>{4}, symbol_set{"x", "y", "z"})
                            == pm_t{4, -8, 12});

                    REQUIRE(monomial_pow(pm_t{-2}, -1, symbol_set{"x"}) == pm_t{2});
                    REQUIRE(monomial_pow(pm_t{-1, 2, -3}, mppp::integer<1>{-1}, symbol_set{"x", "y", "z"})
                            == pm_t{1, -2, 3});
                    REQUIRE(monomial_pow(pm_t{1, -2, 3}, -2, symbol_set{"x", "y", "z"}) == pm_t{-2, 4, -6});
                    REQUIRE(monomial_pow(pm_t{1, -2, 3}, mppp::integer<2>{-4}, symbol_set{"x", "y", "z"})
                            == pm_t{-4, 8, -12});

                    OBAKE_REQUIRES_THROWS_CONTAINS(monomial_pow(pm_t{detail::limits_min<int_t>}, 2, symbol_set{"x"}),
                                                   std::overflow_error, "");

                    if constexpr (bw != static_cast<unsigned>(detail::limits_digits<int_t>)) {
                        OBAKE_REQUIRES_THROWS_CONTAINS(
                            monomial_pow(pm_t{detail::k_packing_get_climits<int_t>(bw, 0)[0],
                                              detail::k_packing_get_climits<int_t>(bw, 1)[0]},
                                         2, symbol_set{"x", "y"}),
                            std::overflow_error, "");

                        OBAKE_REQUIRES_THROWS_CONTAINS(
                            monomial_pow(pm_t{detail::k_packing_get_climits<int_t>(bw, 0)[1],
                                              detail::k_packing_get_climits<int_t>(bw, 1)[1]},
                                         2, symbol_set{"x", "y"}),
                            std::overflow_error, "");
                    }
                } else {
                    if constexpr (bw != static_cast<unsigned>(detail::limits_digits<int_t>)) {
                        OBAKE_REQUIRES_THROWS_CONTAINS(monomial_pow(pm_t{detail::k_packing_get_climits<int_t>(bw, 0),
                                                                         detail::k_packing_get_climits<int_t>(bw, 1)},
                                                                    2, symbol_set{"x", "y"}),
                                                       std::overflow_error, "");
                    }
                }
            }
        });
    });
}
