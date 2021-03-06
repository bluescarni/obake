// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdint>
#include <initializer_list>
#include <list>
#include <random>
#include <tuple>
#include <type_traits>
#include <vector>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/kpack.hpp>
#include <obake/polynomials/d_packed_monomial.hpp>
#include <obake/polynomials/monomial_range_overflow_check.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

static std::mt19937 rng;

using namespace obake;

using int_types = std::tuple<std::int32_t, std::uint32_t
#if defined(OBAKE_PACKABLE_INT64)
                             ,
                             std::int64_t, std::uint64_t
#endif
                             >;

// The packed sizes over which we will be testing for type T.
template <typename T>
using psizes
    = std::tuple<std::integral_constant<unsigned, polynomials::dpm_default_psize>, std::integral_constant<unsigned, 1>,
                 std::integral_constant<unsigned, 2>, std::integral_constant<unsigned, 3>,
                 std::integral_constant<unsigned, detail::kpack_max_size<T>()>>;

TEST_CASE("degree_overflow_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        using pm_t = d_packed_monomial<int_t, 1>;

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
            v1.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2 + 1, detail::limits_min<int_t> / 2 + 1});
            l1.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2 + 1, detail::limits_min<int_t> / 2 + 1});
            v2.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2 + 1, detail::limits_min<int_t> / 2 + 1});
            l2.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2 + 1, detail::limits_min<int_t> / 2 + 1});

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

            v1.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2 + 1, detail::limits_min<int_t> / 2 + 1});
            l1.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2 + 1, detail::limits_min<int_t> / 2 + 1});
            v2.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2 + 1, detail::limits_min<int_t> / 2 + 1});
            l2.emplace_back(std::vector<int_t>{detail::limits_min<int_t> / 2 + 1, detail::limits_min<int_t> / 2 + 1});

            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
            REQUIRE(!monomial_range_overflow_check(v2, v1, ss));
            REQUIRE(!monomial_range_overflow_check(l1, l2, ss));
            REQUIRE(!monomial_range_overflow_check(l2, l1, ss));
            REQUIRE(!monomial_range_overflow_check(v1, l2, ss));
            REQUIRE(!monomial_range_overflow_check(l2, v1, ss));
        }
    });
}

// Test case for a bug in the degree overflow check.
TEST_CASE("degree_overflow_test_bug00")
{
    {
        using int_t = unsigned;
        constexpr auto bw = 1u;
        using pm_t = d_packed_monomial<int_t, bw>;

        pm_t r0[] = {pm_t{0, 0, 0, 0}, pm_t{detail::limits_max<int_t> / 3u, detail::limits_max<int_t> / 3u,
                                            detail::limits_max<int_t> / 3u, detail::limits_max<int_t> / 3u}};
        pm_t r1[] = {pm_t{0, 0, 0, 0}, pm_t{detail::limits_max<int_t> / 3u, detail::limits_max<int_t> / 3u,
                                            detail::limits_max<int_t> / 3u, detail::limits_max<int_t> / 3u}};

        REQUIRE(!monomial_range_overflow_check(r0, r1, symbol_set{"t", "x", "y", "z"}));
    }

    {
        using int_t = int;
        constexpr auto bw = 1u;
        using pm_t = d_packed_monomial<int_t, bw>;

        pm_t r0[] = {pm_t{0, 0, 0, 0}, pm_t{detail::limits_max<int_t> / 3, detail::limits_max<int_t> / 3,
                                            detail::limits_max<int_t> / 3, detail::limits_max<int_t> / 3}};
        pm_t r1[] = {pm_t{0, 0, 0, 0}, pm_t{detail::limits_max<int_t> / 3, detail::limits_max<int_t> / 3,
                                            detail::limits_max<int_t> / 3, detail::limits_max<int_t> / 3}};

        REQUIRE(!monomial_range_overflow_check(r0, r1, symbol_set{"t", "x", "y", "z"}));
    }

    {
        using int_t = int;
        constexpr auto bw = 1u;
        using pm_t = d_packed_monomial<int_t, bw>;

        pm_t r0[] = {pm_t{0, 0, 0, 0}, pm_t{detail::limits_min<int_t> / 3, detail::limits_min<int_t> / 3,
                                            detail::limits_min<int_t> / 3, detail::limits_min<int_t> / 3}};
        pm_t r1[] = {pm_t{0, 0, 0, 0}, pm_t{detail::limits_min<int_t> / 3, detail::limits_min<int_t> / 3,
                                            detail::limits_min<int_t> / 3, detail::limits_min<int_t> / 3}};

        REQUIRE(!monomial_range_overflow_check(r0, r1, symbol_set{"t", "x", "y", "z"}));
    }

    // Do it for the multithreaded case as well.
    {
        using int_t = unsigned;
        constexpr auto bw = 1u;
        using pm_t = d_packed_monomial<int_t, bw>;

        std::vector<pm_t> r0(6000, pm_t{0, 0, 0, 0});
        r0.push_back(pm_t{detail::limits_max<int_t> / 3u, detail::limits_max<int_t> / 3u,
                          detail::limits_max<int_t> / 3u, detail::limits_max<int_t> / 3u});

        std::vector<pm_t> r1(6000, pm_t{0, 0, 0, 0});
        r1.push_back(pm_t{detail::limits_max<int_t> / 3u, detail::limits_max<int_t> / 3u,
                          detail::limits_max<int_t> / 3u, detail::limits_max<int_t> / 3u});

        REQUIRE(!monomial_range_overflow_check(r0, r1, symbol_set{"t", "x", "y", "z"}));
    }

    {
        using int_t = int;
        constexpr auto bw = 1u;
        using pm_t = d_packed_monomial<int_t, bw>;

        std::vector<pm_t> r0(6000, pm_t{0, 0, 0, 0});
        r0.push_back(pm_t{detail::limits_max<int_t> / 3, detail::limits_max<int_t> / 3, detail::limits_max<int_t> / 3,
                          detail::limits_max<int_t> / 3});

        std::vector<pm_t> r1(6000, pm_t{0, 0, 0, 0});
        r1.push_back(pm_t{detail::limits_max<int_t> / 3, detail::limits_max<int_t> / 3, detail::limits_max<int_t> / 3,
                          detail::limits_max<int_t> / 3});

        REQUIRE(!monomial_range_overflow_check(r0, r1, symbol_set{"t", "x", "y", "z"}));
    }

    {
        using int_t = int;
        constexpr auto bw = 1u;
        using pm_t = d_packed_monomial<int_t, bw>;

        std::vector<pm_t> r0(6000, pm_t{0, 0, 0, 0});
        r0.push_back(pm_t{detail::limits_min<int_t> / 3, detail::limits_min<int_t> / 3, detail::limits_min<int_t> / 3,
                          detail::limits_min<int_t> / 3});

        std::vector<pm_t> r1(6000, pm_t{0, 0, 0, 0});
        r1.push_back(pm_t{detail::limits_min<int_t> / 3, detail::limits_min<int_t> / 3, detail::limits_min<int_t> / 3,
                          detail::limits_min<int_t> / 3});

        REQUIRE(!monomial_range_overflow_check(r0, r1, symbol_set{"t", "x", "y", "z"}));
    }
}

// A test for exercising the multi-threaded monomial
// overflow check.
TEST_CASE("mt_overflow_check_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(psizes<int_t>{}, [](auto b) {
            constexpr auto bw = decltype(b)::value;

            if (bw < 2u) {
                return;
            }

            using pm_t = d_packed_monomial<int_t, bw>;
            [[maybe_unused]] constexpr auto psize = pm_t::psize;

            for (auto vs : {3u, 4u, 5u, 6u}) {
                symbol_set ss;
                for (auto j = 0u; j < vs; ++j) {
                    ss.insert(ss.end(), "x_" + detail::to_string(j));
                }

                // Randomly generate a bunch of monomials with
                // exponents within the limits for the given vector size.
                std::vector<pm_t> v1;
                std::list<pm_t> l1;
                std::uniform_int_distribution<int_t> idist;
                std::vector<int_t> tmp(vs);
                for (auto i = 0u; i < 6000u; ++i) {
                    for (auto j = 0u; j < vs; ++j) {
                        const auto [lim_min, lim_max] = detail::kpack_get_lims<int_t>(bw);
                        tmp[j]
                            = idist(rng, typename std::uniform_int_distribution<int_t>::param_type{lim_min, lim_max});
                    }
                    v1.emplace_back(tmp);
                    l1.emplace_back(tmp);
                }
                // Create a range containing a single
                // unitary monomial. This will never
                // overflow when multiplied by v1/l1.
                std::vector<pm_t> v2(1, pm_t(ss));

                REQUIRE(monomial_range_overflow_check(v1, v2, ss));
                REQUIRE(monomial_range_overflow_check(v2, v1, ss));
                REQUIRE(monomial_range_overflow_check(l1, v2, ss));
                REQUIRE(monomial_range_overflow_check(v2, l1, ss));

                // Add monomials with maximal exponents.
                for (auto j = 0u; j < vs; ++j) {
                    const auto [lim_min, lim_max] = detail::kpack_get_lims<int_t>(bw);
                    tmp[j] = lim_max;
                }
                v2[0] = pm_t(tmp);
                for (auto j = 0u; j < vs; ++j) {
                    const auto [lim_min, lim_max] = detail::kpack_get_lims<int_t>(bw);
                    tmp[j] = lim_max;
                }
                v1.emplace_back(tmp);
                l1.emplace_back(tmp);

                REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
                REQUIRE(!monomial_range_overflow_check(l1, v2, ss));
                REQUIRE(!monomial_range_overflow_check(v2, v1, ss));
                REQUIRE(!monomial_range_overflow_check(v2, l1, ss));
            }
        });
    });
}
