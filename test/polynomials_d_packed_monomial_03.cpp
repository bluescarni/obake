// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <initializer_list>
#include <list>
#include <random>
#include <tuple>
#include <type_traits>
#include <vector>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/polynomials/d_packed_monomial.hpp>
#include <obake/polynomials/monomial_range_overflow_check.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

static std::mt19937 rng;

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

TEST_CASE("degree_overflow_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

#if defined(OBAKE_HAVE_GCC_INT128)
        if constexpr (!std::is_same_v<int_t, __int128_t> && !std::is_same_v<int_t, __uint128_t>) {
#endif
            // NOTE: for testing degree overflow, the only option is
            // a bit width equal to the bit width of the type.
            constexpr auto bw = detail::limits_digits<int_t>;
            using pm_t = d_packed_monomial<int_t, bw>;

            symbol_set ss{"x", "y"};

            // Start with short ranges, no overflow.
            std::vector<pm_t> v1, v2;
            std::list<pm_t> l1, l2;

            v1.emplace_back(std::vector<int_t>{1, 2});
            l1.emplace_back(std::vector<int_t>{1, 2});
            v2.emplace_back(std::vector<int_t>{3, 4});
            l2.emplace_back(std::vector<int_t>{3, 4});

            REQUIRE(monomial_range_overflow_check(v1, v2, ss));
            REQUIRE(monomial_range_overflow_check(v2, v1, ss));
            REQUIRE(monomial_range_overflow_check(l1, l2, ss));
            REQUIRE(monomial_range_overflow_check(l2, l1, ss));
            REQUIRE(monomial_range_overflow_check(v1, l2, ss));
            REQUIRE(monomial_range_overflow_check(l2, v1, ss));

            // Short ranges, overflow.
            v1.clear();
            v2.clear();
            l1.clear();
            l2.clear();

            // NOTE: the components do not overflow, and the degrees
            // of each range do not overflow. The degree of the product,
            // however, will overflow.
            v1.emplace_back(std::vector<int_t>{detail::limits_max<int_t> / 2, detail::limits_max<int_t> / 2});
            l1.emplace_back(std::vector<int_t>{detail::limits_max<int_t> / 2, detail::limits_max<int_t> / 2});
            v2.emplace_back(std::vector<int_t>{detail::limits_max<int_t> / 2, detail::limits_max<int_t> / 2});
            l2.emplace_back(std::vector<int_t>{detail::limits_max<int_t> / 2, detail::limits_max<int_t> / 2});

            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
            REQUIRE(!monomial_range_overflow_check(v2, v1, ss));
            REQUIRE(!monomial_range_overflow_check(l1, l2, ss));
            REQUIRE(!monomial_range_overflow_check(l2, l1, ss));
            REQUIRE(!monomial_range_overflow_check(v1, l2, ss));
            REQUIRE(!monomial_range_overflow_check(l2, v1, ss));

            v1.clear();
            v2.clear();
            l1.clear();
            l2.clear();

            if constexpr (is_signed_v<int_t>) {
                v1.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2, detail::limits_min<int_t> / 2});
                l1.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2, detail::limits_min<int_t> / 2});
                v2.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2, detail::limits_min<int_t> / 2});
                l2.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2, detail::limits_min<int_t> / 2});

                REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                REQUIRE(!monomial_range_overflow_check(v2, v1, ss));
                REQUIRE(!monomial_range_overflow_check(l1, l2, ss));
                REQUIRE(!monomial_range_overflow_check(l2, l1, ss));
                REQUIRE(!monomial_range_overflow_check(v1, l2, ss));
                REQUIRE(!monomial_range_overflow_check(l2, v1, ss));

                v1.clear();
                v2.clear();
                l1.clear();
                l2.clear();
            }

            // Try with longer ranges.
            std::uniform_int_distribution<int_t> idist;
            using param_t = typename decltype(idist)::param_type;

            for (auto i = 0u; i < 6000u; ++i) {
                if constexpr (is_signed_v<int_t>) {
                    v1.emplace_back(std::vector<int_t>{idist(rng, param_t{-5, 5}), idist(rng, param_t{-5, 5})});
                    l1.emplace_back(v1.back());
                } else {
                    v1.emplace_back(std::vector<int_t>{idist(rng, param_t{0u, 10u}), idist(rng, param_t{0u, 10u})});
                    l1.emplace_back(v1.back());
                }
            }

            v1.emplace_back(std::vector<int_t>{detail::limits_max<int_t> / 2, detail::limits_max<int_t> / 2});
            l1.emplace_back(std::vector<int_t>{detail::limits_max<int_t> / 2, detail::limits_max<int_t> / 2});
            v2.emplace_back(std::vector<int_t>{detail::limits_max<int_t> / 2, detail::limits_max<int_t> / 2});
            l2.emplace_back(std::vector<int_t>{detail::limits_max<int_t> / 2, detail::limits_max<int_t> / 2});

            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
            REQUIRE(!monomial_range_overflow_check(v2, v1, ss));
            REQUIRE(!monomial_range_overflow_check(l1, l2, ss));
            REQUIRE(!monomial_range_overflow_check(l2, l1, ss));
            REQUIRE(!monomial_range_overflow_check(v1, l2, ss));
            REQUIRE(!monomial_range_overflow_check(l2, v1, ss));

            v1.clear();
            v2.clear();
            l1.clear();
            l2.clear();

            if constexpr (is_signed_v<int_t>) {
                for (auto i = 0u; i < 6000u; ++i) {
                    v1.emplace_back(std::vector<int_t>{idist(rng, param_t{-5, 5}), idist(rng, param_t{-5, 5})});
                    l1.emplace_back(v1.back());
                }

                v1.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2, detail::limits_min<int_t> / 2});
                l1.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2, detail::limits_min<int_t> / 2});
                v2.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2, detail::limits_min<int_t> / 2});
                l2.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2, detail::limits_min<int_t> / 2});

                REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                REQUIRE(!monomial_range_overflow_check(v2, v1, ss));
                REQUIRE(!monomial_range_overflow_check(l1, l2, ss));
                REQUIRE(!monomial_range_overflow_check(l2, l1, ss));
                REQUIRE(!monomial_range_overflow_check(v1, l2, ss));
                REQUIRE(!monomial_range_overflow_check(l2, v1, ss));
            }
#if defined(OBAKE_HAVE_GCC_INT128)
        }
#endif
    });
}
