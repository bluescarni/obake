// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <initializer_list>
#include <random>
#include <tuple>
#include <type_traits>
#include <vector>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/k_packing.hpp>
#include <obake/polynomials/d_packed_monomial.hpp>
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

template <typename T>
using bits_widths = std::tuple<std::integral_constant<unsigned, 3>, std::integral_constant<unsigned, 6>,
                               std::integral_constant<unsigned, detail::limits_digits<T> / 2>,
                               std::integral_constant<unsigned, detail::limits_digits<T>>>;

std::mt19937 rng;

TEST_CASE("basic_tests")
{
    obake_test::disable_slow_stack_traces();

    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(bits_widths<int_t>{}, [](auto b) {
            constexpr auto bw = decltype(b)::value;
            using pm_t = d_packed_monomial<int_t, bw>;
            using c_t = typename pm_t::container_t;

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

            // Constructor from input iterator.
            int_t arr[] = {1, 1, 1};

            // Try empty size first.
            REQUIRE(pm_t(arr, 0) == pm_t{});

            REQUIRE(pm_t(arr, 1)._container() == c_t{1});
            if constexpr (bw == static_cast<unsigned>(detail::limits_digits<int_t>)) {
                REQUIRE(pm_t(arr, 3)._container() == c_t{1, 1, 1});
            } else if constexpr (bw == 3u) {
                REQUIRE(pm_t(arr, 3)._container().size() == 1u);
            }

            // Random testing.
#if defined(OBAKE_HAVE_GCC_INT128)
            if constexpr (bw == 6u && !std::is_same_v<__int128_t, int_t> && !std::is_same_v<__uint128_t, int_t>)
#endif
            {
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

                    pm_t pm(tmp.data(), i);
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
