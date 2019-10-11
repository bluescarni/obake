// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <initializer_list>
#include <random>
#include <tuple>
#include <type_traits>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/hash.hpp>
#include <obake/key/key_merge_symbols.hpp>
#include <obake/polynomials/d_packed_monomial.hpp>
#include <obake/polynomials/monomial_homomorphic_hash.hpp>
#include <obake/polynomials/monomial_mul.hpp>
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
