// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
// #include <bitset>
// #include <cstddef>
#include <cstdint>
#include <initializer_list>
// #include <iostream>
// #include <limits>
// #include <random>
#include <stdexcept>
// #include <sstream>
#include <tuple>
#include <type_traits>
#include <vector>

#include <obake/config.hpp>
// #include <obake/detail/to_string.hpp>
#include <obake/detail/tuple_for_each.hpp>
// #include <obake/hash.hpp>
// #include <obake/key/key_is_compatible.hpp>
// #include <obake/key/key_is_one.hpp>
// #include <obake/key/key_is_zero.hpp>
// #include <obake/key/key_stream_insert.hpp>
// #include <obake/key/key_tex_stream_insert.hpp>
#include <obake/kpack.hpp>
#include <obake/poisson_series/d_packed_trig_monomial.hpp>
#include <obake/symbols.hpp>
// #include <obake/type_name.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

using int_types = std::tuple<std::int32_t
#if defined(OBAKE_PACKABLE_INT64)
                             ,
                             std::int64_t
#endif
                             >;

// The packed sizes over which we will be testing for type T.
template <typename T>
using psizes
    = std::tuple<std::integral_constant<unsigned, 1>, std::integral_constant<unsigned, 2>,
                 std::integral_constant<unsigned, 3>, std::integral_constant<unsigned, detail::kpack_max_size<T>()>>;

// std::mt19937 rng;

TEST_CASE("basic_test")
{
    obake_test::disable_slow_stack_traces();

    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;

        detail::tuple_for_each(psizes<int_t>{}, [](auto b) {
            constexpr auto bw = decltype(b)::value;
            using pm_t = d_packed_trig_monomial<int_t, bw>;

            REQUIRE(!std::is_constructible_v<pm_t, void>);
            REQUIRE(!std::is_constructible_v<pm_t, int>);
            REQUIRE(!std::is_constructible_v<pm_t, const double &>);

            pm_t t00;

            REQUIRE(t00._container().empty());
            REQUIRE(t00.type() == true);

            t00 = pm_t(symbol_set{});
            REQUIRE(t00._container().empty());
            REQUIRE(t00.type() == true);

            t00 = pm_t(symbol_set{"x"});
            REQUIRE(t00._container().size() == 1u);
            REQUIRE(t00._container()[0] == 0);
            REQUIRE(t00.type() == true);

            t00 = pm_t(symbol_set{"x"}, false);
            REQUIRE(t00._container().size() == 1u);
            REQUIRE(t00._container()[0] == 0);
            REQUIRE(t00.type() == false);

            t00 = pm_t(symbol_set{"x", "y"});
            REQUIRE(t00._container().size() == (bw == 1u ? 2u : 1u));
            REQUIRE(std::all_of(t00._container().begin(), t00._container().end(), [](auto x) { return x == 0; }));
            REQUIRE(t00.type() == true);

            t00 = pm_t(symbol_set{"x", "y"}, false);
            REQUIRE(t00._container().size() == (bw == 1u ? 2u : 1u));
            REQUIRE(std::all_of(t00._container().begin(), t00._container().end(), [](auto x) { return x == 0; }));
            REQUIRE(t00.type() == false);

            std::vector<int_t> upack_v;
            auto upack = [&upack_v](const pm_t &t, unsigned size) {
                upack_v.clear();

                int_t tmp;

                auto i = 0u;
                for (const auto &n : t._container()) {
                    kunpacker<int_t> ku(n, pm_t::psize);

                    for (auto j = 0u; j < pm_t::psize && i != size; ++j, ++i) {
                        ku >> tmp;

                        upack_v.push_back(tmp);
                    }
                }
            };

            t00 = pm_t(static_cast<int_t *>(nullptr), 0);
            REQUIRE(t00._container().empty());
            REQUIRE(t00.type() == true);

            t00 = pm_t(static_cast<int_t *>(nullptr), 0, false);
            REQUIRE(t00._container().empty());
            REQUIRE(t00.type() == false);

            upack_v = std::vector<int_t>{1};
            t00 = pm_t(upack_v.data(), 1);
            upack(t00, 1);
            REQUIRE(upack_v == std::vector<int_t>{1});
            REQUIRE(t00.type() == true);

            upack_v = std::vector<int_t>{2};
            t00 = pm_t(upack_v.data(), 1, false);
            upack(t00, 1);
            REQUIRE(upack_v == std::vector<int_t>{2});
            REQUIRE(t00.type() == false);

            upack_v = std::vector<int_t>{1, -1, 3, 3};
            t00 = pm_t(upack_v.data(), 4);
            upack(t00, 4);
            REQUIRE(upack_v == std::vector<int_t>{1, -1, 3, 3});
            REQUIRE(t00.type() == true);

            upack_v = std::vector<int_t>{0, 0, 3, 3};
            t00 = pm_t(upack_v.data(), 4, false);
            upack(t00, 4);
            REQUIRE(upack_v == std::vector<int_t>{0, 0, 3, 3});
            REQUIRE(t00.type() == false);

            upack_v = std::vector<int_t>{0, 0, 3, 3};
            t00 = pm_t(upack_v.data(), 4);
            upack(t00, 4);
            REQUIRE(upack_v == std::vector<int_t>{0, 0, 3, 3});
            REQUIRE(t00.type() == true);

            upack_v = std::vector<int_t>{0, 0, 3, 3};
            t00 = pm_t(upack_v.data(), 4, false);
            upack(t00, 4);
            REQUIRE(upack_v == std::vector<int_t>{0, 0, 3, 3});
            REQUIRE(t00.type() == false);

            upack_v = std::vector<int_t>{-1, 0, 3, 3};
            OBAKE_REQUIRES_THROWS_CONTAINS(pm_t(upack_v.data(), 4), std::invalid_argument,
                                           "Cannot construct a trigonometric monomial whose first nonzero "
                                           "exponent (-1) is negative");

            upack_v = std::vector<int_t>{0, 0, -3, 3};
            OBAKE_REQUIRES_THROWS_CONTAINS(pm_t(upack_v.data(), 4, false), std::invalid_argument,
                                           "Cannot construct a trigonometric monomial whose first nonzero "
                                           "exponent (-3) is negative");

#if 0
            using c_t = typename pm_t::container_t;

            REQUIRE(!std::is_constructible_v<pm_t, void>);
            REQUIRE(!std::is_constructible_v<pm_t, int>);
            REQUIRE(!std::is_constructible_v<pm_t, const double &>);

            // Default ctor.
            REQUIRE(pm_t{}._container().empty());

            // Ctor from symbol set.
            REQUIRE(pm_t{symbol_set{}}._container().empty());
            REQUIRE(pm_t{symbol_set{"x"}}._container() == c_t{0});
            if constexpr (bw == 1u) {
                // With full width, we need an element in the container per symbol.
                REQUIRE(pm_t{symbol_set{"x", "y"}}._container() == c_t{0, 0});
                REQUIRE(pm_t{symbol_set{"x", "y", "z"}}._container() == c_t{0, 0, 0});
            } else {
                REQUIRE(pm_t{symbol_set{"x", "y"}}._container()
                        == c_t(polynomials::detail::dpm_nexpos_to_vsize<pm_t>(2u)));
                REQUIRE(pm_t{symbol_set{"x", "y", "z"}}._container()
                        == c_t(polynomials::detail::dpm_nexpos_to_vsize<pm_t>(3u)));
            }

            // Constructors from iterators.
            int_t arr[] = {1, 1, 1};

            // Try empty size first.
            REQUIRE(pm_t(arr, 0) == pm_t{});
            REQUIRE(pm_t(arr, arr) == pm_t{});

            REQUIRE(pm_t(arr, 1)._container() == c_t{1});
            REQUIRE(pm_t(arr, arr + 1)._container() == c_t{1});
            if constexpr (bw == 1u) {
                REQUIRE(pm_t(arr, 3)._container() == c_t{1, 1, 1});
                REQUIRE(pm_t(arr, arr + 3)._container() == c_t{1, 1, 1});
            } else {
                REQUIRE(pm_t(arr, 3)._container().size() == polynomials::detail::dpm_nexpos_to_vsize<pm_t>(3u));
                REQUIRE(pm_t(arr, arr + 3)._container().size() == polynomials::detail::dpm_nexpos_to_vsize<pm_t>(3u));
            }

            // Try the init list ctor as well.
            if constexpr (bw == 1u) {
                REQUIRE(pm_t{1, 1, 1}._container() == c_t{1, 1, 1});
            } else {
                REQUIRE(pm_t{1, 1, 1}._container().size() == polynomials::detail::dpm_nexpos_to_vsize<pm_t>(3u));
            }

            // Random testing.
            if constexpr (bw <= 3u) {
                using idist_t = std::uniform_int_distribution<detail::make_dependent_t<int_t, decltype(b)>>;
                using param_t = typename idist_t::param_type;
                idist_t dist;

                c_t tmp, cmp;
                int_t tmp_n;

                for (auto i = 0u; i < 1000u; ++i) {
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
                        kunpacker<int_t> ku(n, pm.psize);
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
                        kunpacker<int_t> ku(n, pm.psize);
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
                        kunpacker<int_t> ku(n, pm.psize);
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
#endif
        });
    });
}
