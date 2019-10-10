// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <random>
#include <tuple>
#include <type_traits>
#include <vector>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/hash.hpp>
#include <obake/k_packing.hpp>
#include <obake/key/key_is_compatible.hpp>
#include <obake/key/key_is_one.hpp>
#include <obake/key/key_is_zero.hpp>
#include <obake/polynomials/d_packed_monomial.hpp>
#include <obake/polynomials/monomial_homomorphic_hash.hpp>
#include <obake/symbols.hpp>
#include <obake/type_name.hpp>
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

TEST_CASE("basic_test")
{
    obake_test::disable_slow_stack_traces();

    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto b) {
            constexpr auto bw = decltype(b)::value;
            using pm_t = d_packed_monomial<int_t, bw>;
            using c_t = typename pm_t::container_t;

            REQUIRE(!std::is_constructible_v<pm_t, void>);
            REQUIRE(!std::is_constructible_v<pm_t, int>);
            REQUIRE(!std::is_constructible_v<pm_t, const double &>);

            // Default ctor.
            REQUIRE(pm_t{}._container().empty());

            // Ctor from symbol set.
            REQUIRE(pm_t{symbol_set{}}._container().empty());
            REQUIRE(pm_t{symbol_set{"x"}}._container() == c_t{0});
            if constexpr (bw == static_cast<unsigned>(detail::limits_digits<int_t>)) {
                // With full width, we need an element in the container per symbol.
                REQUIRE(pm_t{symbol_set{"x", "y"}}._container() == c_t{0, 0});
                REQUIRE(pm_t{symbol_set{"x", "y", "z"}}._container() == c_t{0, 0, 0});
            } else if constexpr (bw == 3u) {
                // With 3 bits of width, we can pack into a single value.
                REQUIRE(pm_t{symbol_set{"x", "y"}}._container() == c_t{0});
                REQUIRE(pm_t{symbol_set{"x", "y", "z"}}._container() == c_t{0});
            }

            // Constructors from iterators.
            int_t arr[] = {1, 1, 1};

            // Try empty size first.
            REQUIRE(pm_t(arr, 0) == pm_t{});
            REQUIRE(pm_t(arr, arr) == pm_t{});

            REQUIRE(pm_t(arr, 1)._container() == c_t{1});
            REQUIRE(pm_t(arr, arr + 1)._container() == c_t{1});
            if constexpr (bw == static_cast<unsigned>(detail::limits_digits<int_t>)) {
                REQUIRE(pm_t(arr, 3)._container() == c_t{1, 1, 1});
                REQUIRE(pm_t(arr, arr + 3)._container() == c_t{1, 1, 1});
            } else if constexpr (bw == 3u) {
                REQUIRE(pm_t(arr, 3)._container().size() == 1u);
                REQUIRE(pm_t(arr, arr + 3)._container().size() == 1u);
            }

            // Try the init list ctor as well.
            if constexpr (bw == static_cast<unsigned>(detail::limits_digits<int_t>)) {
                REQUIRE(pm_t{1, 1, 1}._container() == c_t{1, 1, 1});
            } else if constexpr (bw == 3u) {
                REQUIRE(pm_t{1, 1, 1}._container().size() == 1u);
            }

            // Random testing.
            if constexpr (bw >= 6u
#if defined(OBAKE_HAVE_GCC_INT128)
                          && !std::is_same_v<__int128_t, int_t> && !std::is_same_v<__uint128_t, int_t>
#endif
            ) {
                using idist_t = std::uniform_int_distribution<int_t>;
                using param_t = typename idist_t::param_type;
                idist_t dist;

                c_t tmp, cmp;
                int_t tmp_n;

                for (auto i = 0u; i < 100u; ++i) {
                    tmp.resize(i);

                    for (auto j = 0u; j < i; ++j) {
                        if constexpr (is_signed_v<int_t>) {
                            tmp[j] = dist(rng, param_t{-10, 10});
                        } else {
                            tmp[j] = dist(rng, param_t{0, 20});
                        }
                    }

                    // Construct the monomial.
                    pm_t pm(tmp.data(), i);

                    // Unpack it into cmp.
                    cmp.clear();
                    for (const auto &n : pm._container()) {
                        k_unpacker<int_t> ku(n, pm.psize);
                        for (auto j = 0u; j < pm.psize; ++j) {
                            ku >> tmp_n;
                            cmp.push_back(tmp_n);
                        }
                    }

                    // Verify.
                    REQUIRE(cmp.size() >= tmp.size());
                    REQUIRE(std::equal(tmp.begin(), tmp.end(), cmp.begin()));
                    REQUIRE(std::all_of(cmp.data() + tmp.size(), cmp.data() + cmp.size(),
                                        [](const auto &n) { return n == int_t(0); }));

                    // Do the same with input iterators.
                    pm = pm_t(tmp.begin(), tmp.end());

                    cmp.clear();
                    for (const auto &n : pm._container()) {
                        k_unpacker<int_t> ku(n, pm.psize);
                        for (auto j = 0u; j < pm.psize; ++j) {
                            ku >> tmp_n;
                            cmp.push_back(tmp_n);
                        }
                    }

                    REQUIRE(cmp.size() >= tmp.size());
                    REQUIRE(std::equal(tmp.begin(), tmp.end(), cmp.begin()));
                    REQUIRE(std::all_of(cmp.data() + tmp.size(), cmp.data() + cmp.size(),
                                        [](const auto &n) { return n == int_t(0); }));

                    // Do the same with input range.
                    pm = pm_t(tmp);

                    cmp.clear();
                    for (const auto &n : pm._container()) {
                        k_unpacker<int_t> ku(n, pm.psize);
                        for (auto j = 0u; j < pm.psize; ++j) {
                            ku >> tmp_n;
                            cmp.push_back(tmp_n);
                        }
                    }

                    REQUIRE(cmp.size() >= tmp.size());
                    REQUIRE(std::equal(tmp.begin(), tmp.end(), cmp.begin()));
                    REQUIRE(std::all_of(cmp.data() + tmp.size(), cmp.data() + cmp.size(),
                                        [](const auto &n) { return n == int_t(0); }));
                }
            }
        });
    });
}

TEST_CASE("key_is_zero_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto b) {
            constexpr auto bw = decltype(b)::value;
            using pm_t = d_packed_monomial<int_t, bw>;

            REQUIRE(is_zero_testable_key_v<pm_t>);
            REQUIRE(is_zero_testable_key_v<pm_t &>);
            REQUIRE(is_zero_testable_key_v<const pm_t &>);
            REQUIRE(!key_is_zero(pm_t{}, symbol_set{}));
            REQUIRE(!key_is_zero(pm_t{0, 1, 0}, symbol_set{"x", "y", "z"}));
        });
    });
}

TEST_CASE("key_is_one_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto b) {
            constexpr auto bw = decltype(b)::value;
            using pm_t = d_packed_monomial<int_t, bw>;

            REQUIRE(is_one_testable_key_v<pm_t>);
            REQUIRE(is_one_testable_key_v<pm_t &>);
            REQUIRE(is_one_testable_key_v<const pm_t &>);
            REQUIRE(key_is_one(pm_t{}, symbol_set{}));
            REQUIRE(key_is_one(pm_t{0, 0, 0}, symbol_set{"x", "y", "z"}));
            REQUIRE(!key_is_one(pm_t{0, 1, 0}, symbol_set{"x", "y", "z"}));
        });
    });
}

TEST_CASE("hash_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto b) {
            constexpr auto bw = decltype(b)::value;
            using pm_t = d_packed_monomial<int_t, bw>;

            REQUIRE(is_hashable_v<pm_t>);
            REQUIRE(is_hashable_v<pm_t &>);
            REQUIRE(is_hashable_v<const pm_t &>);

            REQUIRE(hash(pm_t{}) == 0u);

            // Generate and print a few random hashes.
            if constexpr (bw == 6u
#if defined(OBAKE_HAVE_GCC_INT128)
                          && !std::is_same_v<__int128_t, int_t> && !std::is_same_v<__uint128_t, int_t>
#endif
            ) {
                using idist_t = std::uniform_int_distribution<int_t>;
                using param_t = typename idist_t::param_type;
                idist_t dist;

                std::vector<int_t> tmp;

                for (auto i = 0u; i < 50u; ++i) {
                    tmp.resize(i);

                    for (auto j = 0u; j < i; ++j) {
                        if constexpr (is_signed_v<int_t>) {
                            tmp[j] = dist(rng, param_t{-10, 10});
                        } else {
                            tmp[j] = dist(rng, param_t{0, 20});
                        }
                    }

                    std::cout << "Hash for type " << type_name<int_t>() << ", bit width " << bw << ", size " << i
                              << ": "
                              << std::bitset<std::numeric_limits<std::size_t>::digits>(
                                     hash(pm_t(tmp.begin(), tmp.end())))
                              << '\n';
                }
            }
        });
    });
}

TEST_CASE("compatibility_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto b) {
            constexpr auto bw = decltype(b)::value;
            using pm_t = d_packed_monomial<int_t, bw>;

            REQUIRE(is_compatibility_testable_key_v<pm_t>);
            REQUIRE(is_compatibility_testable_key_v<pm_t &>);
            REQUIRE(is_compatibility_testable_key_v<const pm_t &>);

            REQUIRE(key_is_compatible(pm_t{}, symbol_set{}));
            REQUIRE(!key_is_compatible(pm_t{}, symbol_set{"x"}));
            REQUIRE(!key_is_compatible(pm_t{1}, symbol_set{}));
            REQUIRE(key_is_compatible(pm_t{1}, symbol_set{"x"}));
            REQUIRE(key_is_compatible(pm_t{1, 1}, symbol_set{"x", "y"}));

            constexpr auto psize = pm_t::psize;
            std::vector<int_t> tmp;
            symbol_set tmp_ss;
            for (auto i = 0u; i < psize * 2u; ++i) {
                tmp.emplace_back(1);
                tmp_ss.insert(tmp_ss.end(), "x_" + detail::to_string(i));
            }
            REQUIRE(key_is_compatible(pm_t(tmp), tmp_ss));

            if constexpr (psize > 1u) {
                auto tmp_ss2(tmp_ss);
                tmp_ss2.insert(tmp_ss2.end(), "a");

                REQUIRE(!key_is_compatible(pm_t(tmp), tmp_ss2));

                auto tmp2(tmp);
                tmp2.emplace_back(1);

                REQUIRE(!key_is_compatible(pm_t(tmp2), tmp_ss));

                REQUIRE(key_is_compatible(pm_t(tmp2), tmp_ss2));
            }

            if constexpr (psize > 1u) {
                // Try with values exceeding the encoded limits.
                constexpr auto &e_lim = detail::k_packing_get_elimits<int_t>(psize);

                pm_t tmp_pm;

                if constexpr (is_signed_v<int_t>) {
                    if constexpr (e_lim[0] > detail::limits_min<int_t>) {
                        tmp_pm._container().push_back(detail::limits_min<int_t>);
                        REQUIRE(!key_is_compatible(tmp_pm, symbol_set{"x"}));
                        tmp_pm._container().clear();
                    }
                    if constexpr (e_lim[1] < detail::limits_max<int_t>) {
                        tmp_pm._container().push_back(detail::limits_max<int_t>);
                        REQUIRE(!key_is_compatible(tmp_pm, symbol_set{"x"}));
                        tmp_pm._container().clear();
                    }
                } else {
                    if constexpr (e_lim < detail::limits_max<int_t>) {
                        tmp_pm._container().push_back(detail::limits_max<int_t>);
                        REQUIRE(!key_is_compatible(tmp_pm, symbol_set{"x"}));
                        tmp_pm._container().clear();
                    }
                }
            }
        });
    });
}
