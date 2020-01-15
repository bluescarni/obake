// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <bitset>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <random>
#include <sstream>
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
#include <obake/key/key_is_compatible.hpp>
#include <obake/key/key_is_one.hpp>
#include <obake/key/key_is_zero.hpp>
#include <obake/key/key_merge_symbols.hpp>
#include <obake/key/key_p_degree.hpp>
#include <obake/key/key_stream_insert.hpp>
#include <obake/polynomials/monomial_homomorphic_hash.hpp>
#include <obake/polynomials/monomial_mul.hpp>
#include <obake/polynomials/monomial_pow.hpp>
#include <obake/polynomials/monomial_range_overflow_check.hpp>
#include <obake/polynomials/packed_monomial.hpp>
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

static std::mt19937 rng(42);

struct foo {
};

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(push)
#pragma warning(disable : 4307)

#endif

TEST_CASE("ctor_test")
{
    obake_test::disable_slow_stack_traces();

    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using kp_t = k_packer<int_t>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_semi_regular_v<pm_t>);

        // Default ctor.
        pm_t pm0;
        REQUIRE(pm0.get_value() == int_t(0));

        // Constructor from value.
        REQUIRE(pm_t(int_t(0)).get_value() == int_t(0));
        REQUIRE(pm_t(int_t(1)).get_value() == int_t(1));
        REQUIRE(pm_t(int_t(2)).get_value() == int_t(2));
        REQUIRE(pm_t(int_t(42)).get_value() == int_t(42));

        // Constructor from symbol set.
        REQUIRE(key_is_compatible(pm_t(symbol_set{}), symbol_set{}));
        REQUIRE(key_is_compatible(pm_t(symbol_set{"x"}), symbol_set{"x"}));
        REQUIRE(key_is_compatible(pm_t(symbol_set{"x", "y"}), symbol_set{"x", "y"}));
        REQUIRE(key_is_compatible(pm_t(symbol_set{"x", "y", "z"}), symbol_set{"x", "y", "z"}));

        REQUIRE(key_is_one(pm_t(symbol_set{}), symbol_set{}));
        REQUIRE(key_is_one(pm_t(symbol_set{"x"}), symbol_set{"x"}));
        REQUIRE(key_is_one(pm_t(symbol_set{"x", "y"}), symbol_set{"x", "y"}));
        REQUIRE(key_is_one(pm_t(symbol_set{"x", "y", "z"}), symbol_set{"x", "y", "z"}));

        // Ctor from input iterator and size.
        int_t arr[] = {1, 2, 3};
        pm_t pm1(arr, 3);
        kp_t kp1(3);
        kp1 << arr[0] << arr[1] << arr[2];
        REQUIRE(pm1.get_value() == kp1.get());

        // Ctor from pair of fwd iterators.
        pm_t pm2(arr, arr + 3);
        REQUIRE(pm2.get_value() == kp1.get());

        // Ctor from range.
        pm_t pm3(arr);
        REQUIRE(pm3.get_value() == kp1.get());

        // Ctor form init list.
        pm_t pm4{1, 2, 3};
        REQUIRE(pm4.get_value() == kp1.get());

        struct sfoo {
        };

        // Test some type traits.
        REQUIRE(!std::is_constructible_v<pm_t, void>);
        REQUIRE(!std::is_constructible_v<pm_t, sfoo>);
        REQUIRE(std::is_constructible_v<pm_t, std::istream_iterator<char>, unsigned>);
        REQUIRE(!std::is_constructible_v<pm_t, int, unsigned>);
        REQUIRE(!std::is_constructible_v<pm_t, std::istream_iterator<char>, std::istream_iterator<char>>);
        REQUIRE(!std::is_constructible_v<pm_t, std::initializer_list<foo>>);
        REQUIRE(std::is_constructible_v<pm_t, std::initializer_list<std::string>>);
        REQUIRE(std::is_constructible_v<pm_t, std::vector<int>>);
        REQUIRE(!std::is_constructible_v<pm_t, std::vector<std::string>>);
        REQUIRE(std::is_constructible_v<pm_t, std::vector<int>::const_iterator, std::vector<int>::const_iterator>);
        REQUIRE(!std::is_constructible_v<pm_t, std::vector<std::string>::const_iterator,
                                         std::vector<std::string>::const_iterator>);
    });
}

TEST_CASE("key_is_zero_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(!key_is_zero(pm_t{}, symbol_set{}));

        REQUIRE(is_zero_testable_key_v<pm_t>);
        REQUIRE(is_zero_testable_key_v<pm_t &>);
        REQUIRE(is_zero_testable_key_v<const pm_t &>);
        REQUIRE(is_zero_testable_key_v<pm_t &&>);
    });
}

TEST_CASE("key_is_one_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(key_is_one(pm_t{}, symbol_set{}));
        REQUIRE(key_is_one(pm_t{0, 0, 0}, symbol_set{"x", "y", "z"}));
        REQUIRE(!key_is_one(pm_t{1, 0, 0}, symbol_set{"x", "y", "z"}));
        REQUIRE(!key_is_one(pm_t{0, 1, 0}, symbol_set{"x", "y", "z"}));
        REQUIRE(!key_is_one(pm_t{0, 0, 1}, symbol_set{"x", "y", "z"}));
        REQUIRE(!key_is_one(pm_t{1, 1, 0}, symbol_set{"x", "y", "z"}));
        REQUIRE(!key_is_one(pm_t{0, 1, 1}, symbol_set{"x", "y", "z"}));
        REQUIRE(!key_is_one(pm_t{1, 0, 1}, symbol_set{"x", "y", "z"}));

        REQUIRE(is_one_testable_key_v<pm_t>);
        REQUIRE(is_one_testable_key_v<pm_t &>);
        REQUIRE(is_one_testable_key_v<const pm_t &>);
        REQUIRE(is_one_testable_key_v<pm_t &&>);
    });
}

TEST_CASE("compare_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_equality_comparable_v<pm_t>);
        REQUIRE(is_equality_comparable_v<pm_t &>);
        REQUIRE(is_equality_comparable_v<const pm_t &>);
        REQUIRE(is_equality_comparable_v<pm_t &&>);

        REQUIRE(pm_t{} == pm_t{});
        REQUIRE(!(pm_t{} != pm_t{}));

        REQUIRE(pm_t{1, 2, 3} == pm_t{1, 2, 3});
        REQUIRE(pm_t{3, 2, 1} != pm_t{1, 2, 3});
    });
}

TEST_CASE("hash_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_hashable_v<pm_t>);
        REQUIRE(is_hashable_v<pm_t &>);
        REQUIRE(is_hashable_v<const pm_t &>);
        REQUIRE(is_hashable_v<pm_t &&>);

        REQUIRE(hash(pm_t{1, 2, 3}) == static_cast<std::size_t>(pm_t{1, 2, 3}.get_value()));
        REQUIRE(hash(pm_t{4, 5, 6}) == static_cast<std::size_t>(pm_t{4, 5, 6}.get_value()));

        // Print a few randomly-generated hash values.
#if defined(OBAKE_HAVE_GCC_INT128)
        if constexpr (!std::is_same_v<int_t, __int128_t> && !std::is_same_v<int_t, __uint128_t>)
#endif
        {
            std::uniform_int_distribution<int_t> idist;
            std::cout << "Int type: " << type_name<int_t>() << '\n';

            std::vector<int_t> v_int;
            for (auto i = 0; i < 6; ++i) {
                if constexpr (is_signed_v<int_t>) {
                    v_int.push_back(idist(rng, typename std::uniform_int_distribution<int_t>::param_type{-2, 2}));
                } else {
                    v_int.push_back(idist(rng, typename std::uniform_int_distribution<int_t>::param_type{0, 5}));
                }
            }

            std::cout << "Hash value: " << std::bitset<std::numeric_limits<std::size_t>::digits>(hash(pm_t(v_int)))
                      << '\n';
        }
    });
}

TEST_CASE("key_is_compatible_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_compatibility_testable_key_v<pm_t>);
        REQUIRE(is_compatibility_testable_key_v<pm_t &>);
        REQUIRE(is_compatibility_testable_key_v<const pm_t &>);
        REQUIRE(is_compatibility_testable_key_v<pm_t &&>);

        REQUIRE(key_is_compatible(pm_t{}, symbol_set{}));
        REQUIRE(key_is_compatible(pm_t{}, symbol_set{"a", "b"}));
        REQUIRE(!key_is_compatible(pm_t{1}, symbol_set{}));
        REQUIRE(!key_is_compatible(pm_t{1, 2}, symbol_set{}));

        if constexpr (is_signed_v<int_t>) {
            // Test with a symbol set with maximum size.
            const auto max_ss_size = detail::k_packing_get_max_size<int_t>();

            symbol_set s;
            for (auto i = 0u; i < max_ss_size; ++i) {
                s.insert("sym_" + std::to_string(i));
            }
            REQUIRE(key_is_compatible(pm_t{}, s));
            // Now make it too large.
            s.insert("x");
            REQUIRE(!key_is_compatible(pm_t{}, s));

            // Test with extremal packed values.
            pm_t p;
            // Size 1.
            p._set_value(detail::limits_min<int_t>);
            REQUIRE(key_is_compatible(p, symbol_set{"a"}));
            p._set_value(detail::limits_max<int_t>);
            REQUIRE(key_is_compatible(p, symbol_set{"a"}));

            // Size 2.
            {
                const auto &e_lim = std::get<3>(
                    detail::k_packing_data<int_t>)[static_cast<unsigned>(detail::limits_digits<int_t>) / 3u - 2u];
                p._set_value(e_lim[0]);
                REQUIRE(key_is_compatible(p, symbol_set{"a", "b"}));
                p._set_value(e_lim[1]);
                REQUIRE(key_is_compatible(p, symbol_set{"a", "b"}));
            }

            // Size 3.
            {
                const auto &e_lim = std::get<3>(
                    detail::k_packing_data<int_t>)[static_cast<unsigned>(detail::limits_digits<int_t>) / 3u - 3u];
                p._set_value(e_lim[0]);
                REQUIRE(key_is_compatible(p, symbol_set{"a", "b", "c"}));
                p._set_value(e_lim[1]);
                REQUIRE(key_is_compatible(p, symbol_set{"a", "b", "c"}));
            }

            // Try to go out of the limits, if possible.
            // Size 2.
            {
                const auto &e_lim = std::get<3>(
                    detail::k_packing_data<int_t>)[static_cast<unsigned>(detail::limits_digits<int_t>) / 3u - 2u];
                if (e_lim[0] > detail::limits_min<int_t>) {
                    p._set_value(e_lim[0] - int_t(1));
                    REQUIRE(!key_is_compatible(p, symbol_set{"a", "b"}));
                }
                if (e_lim[1] < detail::limits_max<int_t>) {
                    p._set_value(e_lim[1] + int_t(1));
                    REQUIRE(!key_is_compatible(p, symbol_set{"a", "b"}));
                }
            }

            // Size 3.
            {
                const auto &e_lim = std::get<3>(
                    detail::k_packing_data<int_t>)[static_cast<unsigned>(detail::limits_digits<int_t>) / 3u - 3u];
                if (e_lim[0] > detail::limits_min<int_t>) {
                    p._set_value(e_lim[0] - int_t(1));
                    REQUIRE(!key_is_compatible(p, symbol_set{"a", "b", "c"}));
                }
                if (e_lim[1] < detail::limits_max<int_t>) {
                    p._set_value(e_lim[1] + int_t(1));
                    REQUIRE(!key_is_compatible(p, symbol_set{"a", "b", "c"}));
                }
            }
        } else {
            // Test with a symbol set with maximum size.
            const auto max_ss_size = detail::k_packing_get_max_size<int_t>();

            symbol_set s;
            for (auto i = 0u; i < max_ss_size; ++i) {
                s.insert("sym_" + std::to_string(i));
            }
            REQUIRE(key_is_compatible(pm_t{}, s));
            // Now make it too large.
            s.insert("x");
            REQUIRE(!key_is_compatible(pm_t{}, s));

            // Test with extremal packed values.
            pm_t p;
            // Size 1.
            p._set_value(detail::limits_min<int_t>);
            REQUIRE(key_is_compatible(p, symbol_set{"a"}));
            p._set_value(detail::limits_max<int_t>);
            REQUIRE(key_is_compatible(p, symbol_set{"a"}));

            // Size 2.
            {
                const auto &e_lim = std::get<3>(
                    detail::k_packing_data<int_t>)[static_cast<unsigned>(detail::limits_digits<int_t>) / 3u - 2u];
                p._set_value(e_lim);
                REQUIRE(key_is_compatible(p, symbol_set{"a", "b"}));
            }

            // Size 3.
            {
                const auto &e_lim = std::get<3>(
                    detail::k_packing_data<int_t>)[static_cast<unsigned>(detail::limits_digits<int_t>) / 3u - 3u];
                p._set_value(e_lim);
                REQUIRE(key_is_compatible(p, symbol_set{"a", "b", "c"}));
            }

            // Try to go out of the limits, if possible.
            // Size 2.
            {
                const auto &e_lim = std::get<3>(
                    detail::k_packing_data<int_t>)[static_cast<unsigned>(detail::limits_digits<int_t>) / 3u - 2u];
                if (e_lim < detail::limits_max<int_t>) {
                    p._set_value(e_lim + int_t(1));
                    REQUIRE(!key_is_compatible(p, symbol_set{"a", "b"}));
                }
            }

            // Size 3.
            {
                const auto &e_lim = std::get<3>(
                    detail::k_packing_data<int_t>)[static_cast<unsigned>(detail::limits_digits<int_t>) / 3u - 3u];
                if (e_lim < detail::limits_max<int_t>) {
                    p._set_value(e_lim + int_t(1));
                    REQUIRE(!key_is_compatible(p, symbol_set{"a", "b", "c"}));
                }
            }
        }
    });
}

TEST_CASE("key_stream_insert_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        auto oss_wrap = [](const pm_t &p, const symbol_set &s) {
            std::ostringstream oss;
            key_stream_insert(oss, p, s);
            return oss.str();
        };

        REQUIRE(is_stream_insertable_key_v<pm_t>);
        REQUIRE(is_stream_insertable_key_v<pm_t &>);
        REQUIRE(is_stream_insertable_key_v<pm_t &&>);
        REQUIRE(is_stream_insertable_key_v<const pm_t &>);

        REQUIRE(oss_wrap(pm_t{}, symbol_set{}) == "1");
        REQUIRE(oss_wrap(pm_t{0}, symbol_set{"x"}) == "1");
        REQUIRE(oss_wrap(pm_t{0, 0}, symbol_set{"x", "y"}) == "1");
        REQUIRE(oss_wrap(pm_t{1}, symbol_set{"x"}) == "x");
        REQUIRE(oss_wrap(pm_t{1, 2}, symbol_set{"x", "y"}) == "x*y**2");
        REQUIRE(oss_wrap(pm_t{2, 1}, symbol_set{"x", "y"}) == "x**2*y");
        REQUIRE(oss_wrap(pm_t{0, 1}, symbol_set{"x", "y"}) == "y");
        REQUIRE(oss_wrap(pm_t{0, 2}, symbol_set{"x", "y"}) == "y**2");
        REQUIRE(oss_wrap(pm_t{1, 0}, symbol_set{"x", "y"}) == "x");
        REQUIRE(oss_wrap(pm_t{2, 0}, symbol_set{"x", "y"}) == "x**2");
        REQUIRE(oss_wrap(pm_t{0, 0, 1}, symbol_set{"x", "y", "z"}) == "z");
        REQUIRE(oss_wrap(pm_t{0, 1, 0}, symbol_set{"x", "y", "z"}) == "y");
        REQUIRE(oss_wrap(pm_t{1, 0, 0}, symbol_set{"x", "y", "z"}) == "x");
        REQUIRE(oss_wrap(pm_t{1, 0, 1}, symbol_set{"x", "y", "z"}) == "x*z");
        REQUIRE(oss_wrap(pm_t{0, 1, 1}, symbol_set{"x", "y", "z"}) == "y*z");
        REQUIRE(oss_wrap(pm_t{1, 1, 0}, symbol_set{"x", "y", "z"}) == "x*y");
        REQUIRE(oss_wrap(pm_t{0, 0, 2}, symbol_set{"x", "y", "z"}) == "z**2");
        REQUIRE(oss_wrap(pm_t{0, 2, 0}, symbol_set{"x", "y", "z"}) == "y**2");
        REQUIRE(oss_wrap(pm_t{2, 0, 0}, symbol_set{"x", "y", "z"}) == "x**2");
        REQUIRE(oss_wrap(pm_t{2, 0, 1}, symbol_set{"x", "y", "z"}) == "x**2*z");
        REQUIRE(oss_wrap(pm_t{0, 2, 3}, symbol_set{"x", "y", "z"}) == "y**2*z**3");
        REQUIRE(oss_wrap(pm_t{1, 1, 4}, symbol_set{"x", "y", "z"}) == "x*y*z**4");

        if constexpr (is_signed_v<int_t>) {
            REQUIRE(oss_wrap(pm_t{-1}, symbol_set{"x"}) == "x**-1");
            REQUIRE(oss_wrap(pm_t{-1, 2}, symbol_set{"x", "y"}) == "x**-1*y**2");
            REQUIRE(oss_wrap(pm_t{-2, 1}, symbol_set{"x", "y"}) == "x**-2*y");
            REQUIRE(oss_wrap(pm_t{0, -1}, symbol_set{"x", "y"}) == "y**-1");
            REQUIRE(oss_wrap(pm_t{0, -2}, symbol_set{"x", "y"}) == "y**-2");
            REQUIRE(oss_wrap(pm_t{-1, 0}, symbol_set{"x", "y"}) == "x**-1");
            REQUIRE(oss_wrap(pm_t{-2, 0}, symbol_set{"x", "y"}) == "x**-2");
            REQUIRE(oss_wrap(pm_t{0, 0, -1}, symbol_set{"x", "y", "z"}) == "z**-1");
            REQUIRE(oss_wrap(pm_t{0, -1, 0}, symbol_set{"x", "y", "z"}) == "y**-1");
            REQUIRE(oss_wrap(pm_t{-1, 0, 0}, symbol_set{"x", "y", "z"}) == "x**-1");
            REQUIRE(oss_wrap(pm_t{-1, 0, 1}, symbol_set{"x", "y", "z"}) == "x**-1*z");
            REQUIRE(oss_wrap(pm_t{0, 1, -1}, symbol_set{"x", "y", "z"}) == "y*z**-1");
            REQUIRE(oss_wrap(pm_t{1, -1, 0}, symbol_set{"x", "y", "z"}) == "x*y**-1");
            REQUIRE(oss_wrap(pm_t{0, 0, -2}, symbol_set{"x", "y", "z"}) == "z**-2");
            REQUIRE(oss_wrap(pm_t{0, -2, 0}, symbol_set{"x", "y", "z"}) == "y**-2");
            REQUIRE(oss_wrap(pm_t{-2, 0, 0}, symbol_set{"x", "y", "z"}) == "x**-2");
            REQUIRE(oss_wrap(pm_t{2, 0, -1}, symbol_set{"x", "y", "z"}) == "x**2*z**-1");
            REQUIRE(oss_wrap(pm_t{0, -2, 3}, symbol_set{"x", "y", "z"}) == "y**-2*z**3");
            REQUIRE(oss_wrap(pm_t{1, 1, -4}, symbol_set{"x", "y", "z"}) == "x*y*z**-4");
        }
    });
}

TEST_CASE("key_merge_symbols_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_symbols_mergeable_key_v<pm_t>);
        REQUIRE(is_symbols_mergeable_key_v<pm_t &>);
        REQUIRE(is_symbols_mergeable_key_v<pm_t &&>);
        REQUIRE(is_symbols_mergeable_key_v<const pm_t &>);

        REQUIRE(key_merge_symbols(pm_t{}, symbol_idx_map<symbol_set>{}, symbol_set{}) == pm_t{});
        REQUIRE(key_merge_symbols(pm_t{}, symbol_idx_map<symbol_set>{{0, {"x"}}}, symbol_set{}) == pm_t{0});
        REQUIRE(key_merge_symbols(pm_t{1}, symbol_idx_map<symbol_set>{}, symbol_set{"x"}) == pm_t{1});
        REQUIRE(key_merge_symbols(pm_t{1}, symbol_idx_map<symbol_set>{{0, {"y"}}}, symbol_set{"x"}) == pm_t{0, 1});
        REQUIRE(key_merge_symbols(pm_t{1}, symbol_idx_map<symbol_set>{{1, {"y"}}}, symbol_set{"x"}) == pm_t{1, 0});
        REQUIRE(key_merge_symbols(pm_t{1, 2, 3},
                                  symbol_idx_map<symbol_set>{{0, {"a", "b"}}, {1, {"c"}}, {3, {"d", "e"}}},
                                  symbol_set{"x", "y", "z"})
                == pm_t{0, 0, 1, 0, 2, 3, 0, 0});
        REQUIRE(key_merge_symbols(pm_t{1, 2, 3}, symbol_idx_map<symbol_set>{{3, {"d", "e"}}}, symbol_set{"x", "y", "z"})
                == pm_t{1, 2, 3, 0, 0});
        REQUIRE(key_merge_symbols(pm_t{1, 2, 3}, symbol_idx_map<symbol_set>{{0, {"d", "e"}}}, symbol_set{"x", "y", "z"})
                == pm_t{0, 0, 1, 2, 3});
        REQUIRE(key_merge_symbols(pm_t{1, 2, 3}, symbol_idx_map<symbol_set>{{1, {"d", "e"}}}, symbol_set{"x", "y", "z"})
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
    });
}

TEST_CASE("monomial_mul_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_multipliable_monomial_v<pm_t &, const pm_t &, const pm_t &>);
        REQUIRE(is_multipliable_monomial_v<pm_t &, pm_t &, pm_t &>);
        REQUIRE(is_multipliable_monomial_v<pm_t &, pm_t &&, pm_t &&>);
        REQUIRE(!is_multipliable_monomial_v<const pm_t &, const pm_t &, const pm_t &>);
        REQUIRE(!is_multipliable_monomial_v<pm_t &&, const pm_t &, const pm_t &>);

        pm_t a, b, c;
        monomial_mul(a, b, c, symbol_set{});
        REQUIRE(a == pm_t{});

        b = pm_t{1, 2, 3};
        c = pm_t{4, 5, 6};
        a = pm_t{0, 1, 0};
        monomial_mul(a, b, c, symbol_set{"x", "y", "z"});
        REQUIRE(a == pm_t{5, 7, 9});
    });
}

TEST_CASE("monomial_range_overflow_check")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        std::vector<pm_t> v1, v2;
        symbol_set ss;

        // Empty symbol set.
        REQUIRE(monomial_range_overflow_check(v1, v2, ss));

        // Both empty ranges.
        ss = symbol_set{"x", "y", "z"};
        REQUIRE(monomial_range_overflow_check(v1, v2, ss));

        // Empty second range.
        v1.emplace_back(pm_t{1, 2, 3});
        REQUIRE(monomial_range_overflow_check(v1, v2, ss));

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
        // Get the delta bit width corresponding to a vector size of 3.
        const auto nbits = detail::k_packing_size_to_bits<int_t>(3u);
        // Get the limits of the component at index 2.
        const auto &lims = detail::k_packing_get_climits<int_t>(nbits, 2);
        if constexpr (is_signed_v<int_t>) {
            v1.emplace_back(pm_t{int_t(0), int_t(4), lims[0]});
            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
            v1.pop_back();

            v1.emplace_back(pm_t{int_t(0), int_t(4), lims[1]});
            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
            v1.pop_back();
        } else {
            v1.emplace_back(pm_t{int_t(0), int_t(4), lims});
            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
        }

        // Special-casing for size 1.
        v1.clear();
        v2.clear();
        ss = symbol_set{"x"};

        v1.emplace_back(pm_t{1});
        v2.emplace_back(pm_t{2});
        REQUIRE(monomial_range_overflow_check(v1, v2, ss));
        v1.emplace_back(pm_t{4});
        REQUIRE(monomial_range_overflow_check(v1, v2, ss));
        v1.emplace_back(pm_t{2});
        v1.emplace_back(pm_t{2});
        v1.emplace_back(pm_t{0});
        v2.emplace_back(pm_t{2});
        v2.emplace_back(pm_t{1});
        v2.emplace_back(pm_t{0});
        REQUIRE(monomial_range_overflow_check(v1, v2, ss));

        if constexpr (is_signed_v<int_t>) {
            v1.emplace_back(pm_t{-2});
            v1.emplace_back(pm_t{2});
            v1.emplace_back(pm_t{0});
            v2.emplace_back(pm_t{-2});
            v2.emplace_back(pm_t{1});
            v2.emplace_back(pm_t{0});
            REQUIRE(monomial_range_overflow_check(v1, v2, ss));
        }

        // Overflow check.
        if constexpr (is_signed_v<int_t>) {
            v1.emplace_back(pm_t{detail::limits_min<int_t>});
            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
            v1.pop_back();

            v1.emplace_back(pm_t{detail::limits_max<int_t>});
            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
            v1.pop_back();
        } else {
            v1.emplace_back(pm_t{detail::limits_max<int_t>});
            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
        }

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
    });
}

const int ntrials = 100;

TEST_CASE("homomorphic_hash")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_homomorphically_hashable_monomial_v<pm_t>);
        REQUIRE(!is_homomorphically_hashable_monomial_v<pm_t &>);
        REQUIRE(!is_homomorphically_hashable_monomial_v<pm_t &&>);
        REQUIRE(!is_homomorphically_hashable_monomial_v<const pm_t &>);

#if defined(OBAKE_HAVE_CONCEPTS)
        REQUIRE(HomomorphicallyHashableMonomial<pm_t>);
        REQUIRE(!HomomorphicallyHashableMonomial<pm_t &>);
        REQUIRE(!HomomorphicallyHashableMonomial<pm_t &&>);
        REQUIRE(!HomomorphicallyHashableMonomial<const pm_t &>);
#endif

#if defined(OBAKE_HAVE_GCC_INT128)
        if constexpr (!std::is_same_v<int_t, __int128_t> && !std::is_same_v<int_t, __uint128_t>)
#endif
        {
            std::vector<int_t> v1, v2, v3;
            v1.resize(6u);
            v2.resize(6u);
            v3.resize(6u);

            std::uniform_int_distribution<int_t> idist;

            for (auto k = 0; k < ntrials; ++k) {
                for (auto i = 0u; i < 6u; ++i) {
                    if constexpr (is_signed_v<int_t>) {
                        v1[i] = idist(rng, typename std::uniform_int_distribution<int_t>::param_type{-2, 2});
                        v2[i] = idist(rng, typename std::uniform_int_distribution<int_t>::param_type{-2, 2});
                    } else {
                        v1[i] = idist(rng, typename std::uniform_int_distribution<int_t>::param_type{0, 5});
                        v2[i] = idist(rng, typename std::uniform_int_distribution<int_t>::param_type{0, 5});
                    }
                    v3[i] = v1[i] + v2[i];
                }

                const auto h1 = hash(pm_t(v1));
                const auto h2 = hash(pm_t(v2));
                const auto h3 = hash(pm_t(v3));

                REQUIRE(h1 + h2 == h3);
            }
        }

#if defined(OBAKE_HAVE_GCC_INT128)
        if constexpr (std::is_same_v<int_t, __uint128_t>) {
            const auto max_ss_size = detail::k_packing_get_max_size<int_t>();
            const auto nbits = detail::k_packing_size_to_bits<int_t>(max_ss_size);

            std::vector<int_t> v1, v2, v3;
            v1.resize(max_ss_size);
            v2.resize(max_ss_size);
            v3.resize(max_ss_size);

            for (auto i = 0u; i < max_ss_size; ++i) {
                v1[i] = detail::k_packing_get_climits<int_t>(nbits, i) / 2u;
                v2[i] = detail::k_packing_get_climits<int_t>(nbits, i) / 2u;
                v3[i] = v1[i] + v2[i];
            }

            const auto h1 = hash(pm_t(v1));
            const auto h2 = hash(pm_t(v2));
            const auto h3 = hash(pm_t(v3));

            REQUIRE(h1 + h2 == h3);
        }

        if constexpr (std::is_same_v<int_t, __int128_t>) {
            const auto max_ss_size = detail::k_packing_get_max_size<int_t>();
            const auto nbits = detail::k_packing_size_to_bits<int_t>(max_ss_size);

            std::vector<int_t> v1, v2, v3;
            v1.resize(max_ss_size);
            v2.resize(max_ss_size);
            v3.resize(max_ss_size);

            {
                for (auto i = 0u; i < max_ss_size; ++i) {
                    v1[i] = detail::k_packing_get_climits<int_t>(nbits, i)[0] / 2u;
                    v2[i] = detail::k_packing_get_climits<int_t>(nbits, i)[0] / 2u;
                    v3[i] = v1[i] + v2[i];
                }

                const auto h1 = hash(pm_t(v1));
                const auto h2 = hash(pm_t(v2));
                const auto h3 = hash(pm_t(v3));

                REQUIRE(h1 + h2 == h3);
            }

            {
                for (auto i = 0u; i < max_ss_size; ++i) {
                    v1[i] = detail::k_packing_get_climits<int_t>(nbits, i)[1] / 2u;
                    v2[i] = detail::k_packing_get_climits<int_t>(nbits, i)[1] / 2u;
                    v3[i] = v1[i] + v2[i];
                }

                const auto h1 = hash(pm_t(v1));
                const auto h2 = hash(pm_t(v2));
                const auto h3 = hash(pm_t(v3));

                REQUIRE(h1 + h2 == h3);
            }

            {
                for (auto i = 0u; i < max_ss_size; ++i) {
                    v1[i] = detail::k_packing_get_climits<int_t>(nbits, i)[0] / 2u;
                    v2[i] = detail::k_packing_get_climits<int_t>(nbits, i)[1] / 2u;
                    v3[i] = v1[i] + v2[i];
                }

                const auto h1 = hash(pm_t(v1));
                const auto h2 = hash(pm_t(v2));
                const auto h3 = hash(pm_t(v3));

                REQUIRE(h1 + h2 == h3);
            }
        }
#endif
    });
}

TEST_CASE("key_degree_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(key_degree(pm_t{}, symbol_set{}) == int_t(0));
        REQUIRE(key_degree(pm_t{1}, symbol_set{"x"}) == int_t(1));
        REQUIRE(key_degree(pm_t{42}, symbol_set{"x"}) == int_t(42));

        if constexpr (is_signed_v<int_t>) {
            REQUIRE(key_degree(pm_t{-1}, symbol_set{"x"}) == int_t(-1));
            REQUIRE(key_degree(pm_t{-42}, symbol_set{"x"}) == int_t(-42));
        }

        REQUIRE(key_degree(pm_t{1, 2}, symbol_set{"x", "y"}) == int_t(3));
        REQUIRE(key_degree(pm_t{42, 3}, symbol_set{"x", "y"}) == int_t(45));

        if constexpr (is_signed_v<int_t>) {
            REQUIRE(key_degree(pm_t{-1, 2}, symbol_set{"x", "y"}) == int_t(1));
            REQUIRE(key_degree(pm_t{-42, 5}, symbol_set{"x", "y"}) == int_t(-37));
        }

        REQUIRE(is_key_with_degree_v<pm_t>);
        REQUIRE(is_key_with_degree_v<pm_t &>);
        REQUIRE(is_key_with_degree_v<const pm_t &>);
        REQUIRE(is_key_with_degree_v<pm_t &&>);
    });
}

TEST_CASE("key_p_degree_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(key_p_degree(pm_t{}, symbol_idx_set{}, symbol_set{}) == int_t(0));
        REQUIRE(key_p_degree(pm_t{1}, symbol_idx_set{0}, symbol_set{"x"}) == int_t(1));
        REQUIRE(key_p_degree(pm_t{1}, symbol_idx_set{}, symbol_set{"x"}) == int_t(0));
        REQUIRE(key_p_degree(pm_t{42}, symbol_idx_set{0}, symbol_set{"x"}) == int_t(42));
        REQUIRE(key_p_degree(pm_t{42}, symbol_idx_set{}, symbol_set{"x"}) == int_t(0));

        if constexpr (is_signed_v<int_t>) {
            REQUIRE(key_p_degree(pm_t{-1}, symbol_idx_set{0}, symbol_set{"x"}) == int_t(-1));
            REQUIRE(key_p_degree(pm_t{-1}, symbol_idx_set{}, symbol_set{"x"}) == int_t(0));
            REQUIRE(key_p_degree(pm_t{-42}, symbol_idx_set{0}, symbol_set{"x"}) == int_t(-42));
            REQUIRE(key_p_degree(pm_t{-42}, symbol_idx_set{}, symbol_set{"x"}) == int_t(0));
        }

        REQUIRE(key_p_degree(pm_t{1, 2}, symbol_idx_set{0, 1}, symbol_set{"x", "y"}) == int_t(3));
        REQUIRE(key_p_degree(pm_t{1, 2}, symbol_idx_set{0}, symbol_set{"x", "y"}) == int_t(1));
        REQUIRE(key_p_degree(pm_t{1, 2}, symbol_idx_set{1}, symbol_set{"x", "y"}) == int_t(2));
        REQUIRE(key_p_degree(pm_t{1, 2}, symbol_idx_set{}, symbol_set{"x", "y"}) == int_t(0));
        REQUIRE(key_p_degree(pm_t{42, 3}, symbol_idx_set{0, 1}, symbol_set{"x", "y"}) == int_t(45));
        REQUIRE(key_p_degree(pm_t{42, 3}, symbol_idx_set{0}, symbol_set{"x", "y"}) == int_t(42));
        REQUIRE(key_p_degree(pm_t{42, 3}, symbol_idx_set{1}, symbol_set{"x", "y"}) == int_t(3));
        REQUIRE(key_p_degree(pm_t{42, 3}, symbol_idx_set{}, symbol_set{"x", "y"}) == int_t(0));

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
            REQUIRE(key_p_degree(pm_t{-42, 5}, symbol_idx_set{0, 1}, symbol_set{"x", "y"}) == int_t(-37));
            REQUIRE(key_p_degree(pm_t{-42, 5}, symbol_idx_set{0}, symbol_set{"x", "y"}) == int_t(-42));
            REQUIRE(key_p_degree(pm_t{-42, 5}, symbol_idx_set{1}, symbol_set{"x", "y"}) == int_t(5));
            REQUIRE(key_p_degree(pm_t{-42, 5}, symbol_idx_set{}, symbol_set{"x", "y"}) == int_t(0));
        }

        REQUIRE(is_key_with_p_degree_v<pm_t>);
        REQUIRE(is_key_with_p_degree_v<pm_t &>);
        REQUIRE(is_key_with_p_degree_v<const pm_t &>);
        REQUIRE(is_key_with_p_degree_v<pm_t &&>);
    });
}

TEST_CASE("monomial_pow_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(!is_exponentiable_monomial_v<pm_t, void>);
        REQUIRE(!is_exponentiable_monomial_v<void, pm_t>);

        REQUIRE(is_exponentiable_monomial_v<pm_t, int>);
        REQUIRE(is_exponentiable_monomial_v<const pm_t, int &>);
        REQUIRE(is_exponentiable_monomial_v<const pm_t &, const int &>);
        REQUIRE(is_exponentiable_monomial_v<pm_t &, const int>);
        REQUIRE(!is_exponentiable_monomial_v<pm_t &, std::string>);

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
            monomial_pow(pm_t{1, 2, 3}, mppp::rational<1>{4, 3}, symbol_set{"x", "y", "z"}), std::invalid_argument,
            "Invalid exponent for monomial exponentiation: the exponent (4/3) cannot be converted into an integral "
            "value");

        // Check overflows, both in the single exponent exponentiation and in the coding limits.
        OBAKE_REQUIRES_THROWS_CONTAINS(monomial_pow(pm_t{detail::limits_max<int_t>}, 2, symbol_set{"x"}),
                                       std::overflow_error, "");

        // Get the delta bit width corresponding to a vector size of 2.
        const auto nbits = detail::k_packing_size_to_bits<int_t>(2u);

        if constexpr (is_signed_v<int_t>) {
            REQUIRE(monomial_pow(pm_t{-1}, 0, symbol_set{"x"}) == pm_t{0});
            REQUIRE(monomial_pow(pm_t{-2}, 0, symbol_set{"x"}) == pm_t{0});
            REQUIRE(monomial_pow(pm_t{-2}, 1, symbol_set{"x"}) == pm_t{-2});
            REQUIRE(monomial_pow(pm_t{-1, 2, -3}, 0, symbol_set{"x", "y", "z"}) == pm_t{0, 0, 0});
            REQUIRE(monomial_pow(pm_t{-1, 2, -3}, mppp::integer<1>{1}, symbol_set{"x", "y", "z"}) == pm_t{-1, 2, -3});
            REQUIRE(monomial_pow(pm_t{1, -2, 3}, 2, symbol_set{"x", "y", "z"}) == pm_t{2, -4, 6});
            REQUIRE(monomial_pow(pm_t{1, -2, 3}, mppp::integer<2>{4}, symbol_set{"x", "y", "z"}) == pm_t{4, -8, 12});

            REQUIRE(monomial_pow(pm_t{-2}, -1, symbol_set{"x"}) == pm_t{2});
            REQUIRE(monomial_pow(pm_t{-1, 2, -3}, mppp::integer<1>{-1}, symbol_set{"x", "y", "z"}) == pm_t{1, -2, 3});
            REQUIRE(monomial_pow(pm_t{1, -2, 3}, -2, symbol_set{"x", "y", "z"}) == pm_t{-2, 4, -6});
            REQUIRE(monomial_pow(pm_t{1, -2, 3}, mppp::integer<2>{-4}, symbol_set{"x", "y", "z"}) == pm_t{-4, 8, -12});

            OBAKE_REQUIRES_THROWS_CONTAINS(monomial_pow(pm_t{detail::limits_min<int_t>}, 2, symbol_set{"x"}),
                                           std::overflow_error, "");

            OBAKE_REQUIRES_THROWS_CONTAINS(monomial_pow(pm_t{detail::k_packing_get_climits<int_t>(nbits, 0)[0],
                                                             detail::k_packing_get_climits<int_t>(nbits, 1)[0]},
                                                        2, symbol_set{"x", "y"}),
                                           std::overflow_error, "");

            OBAKE_REQUIRES_THROWS_CONTAINS(monomial_pow(pm_t{detail::k_packing_get_climits<int_t>(nbits, 0)[1],
                                                             detail::k_packing_get_climits<int_t>(nbits, 1)[1]},
                                                        2, symbol_set{"x", "y"}),
                                           std::overflow_error, "");
        } else {
            OBAKE_REQUIRES_THROWS_CONTAINS(monomial_pow(pm_t{detail::k_packing_get_climits<int_t>(nbits, 0),
                                                             detail::k_packing_get_climits<int_t>(nbits, 1)},
                                                        2, symbol_set{"x", "y"}),
                                           std::overflow_error, "");
        }
    });
}

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(pop)

#endif
