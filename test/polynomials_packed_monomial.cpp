// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/polynomials/packed_monomial.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <piranha/config.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/detail/tuple_for_each.hpp>
#include <piranha/hash.hpp>
#include <piranha/key/key_is_compatible.hpp>
#include <piranha/key/key_is_one.hpp>
#include <piranha/key/key_is_zero.hpp>
#include <piranha/key/key_stream_insert.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>
#include <piranha/utils/bit_packing.hpp>

using namespace piranha;

using int_types = std::tuple<int, unsigned, long, unsigned long, long long, unsigned long long
#if defined(PIRANHA_HAVE_GCC_INT128)
                             ,
                             __int128_t, __uint128_t
#endif
                             >;

struct foo {
};

TEST_CASE("ctor_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using bp_t = bit_packer<int_t>;
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
        bp_t bp1(3);
        bp1 << arr[0] << arr[1] << arr[2];
        REQUIRE(pm1.get_value() == bp1.get());

        // Ctor from pair of fwd iterators.
        pm_t pm2(arr, arr + 3);
        REQUIRE(pm2.get_value() == bp1.get());

        // Ctor from range.
        pm_t pm3(arr);
        REQUIRE(pm3.get_value() == bp1.get());

        // Ctor form init list.
        pm_t pm4{1, 2, 3};
        REQUIRE(pm4.get_value() == bp1.get());

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
            const auto &mmp_arr = detail::sbp_get_mmp<int_t>();

            // Test with a maximal symbol set.
            symbol_set s;
            for (decltype(mmp_arr.size()) i = 0; i < mmp_arr.size(); ++i) {
                s.insert("sym_" + std::to_string(i));
            }
            REQUIRE(key_is_compatible(pm_t{}, s));
            // Now make it too large.
            s.insert("x");
            REQUIRE(!key_is_compatible(pm_t{}, s));

            // Test with maximal values for the packed value.
            pm_t p;
            p._set_value(std::get<0>(detail::limits_minmax<int_t>));
            REQUIRE(key_is_compatible(p, symbol_set{"a"}));
            p._set_value(std::get<1>(detail::limits_minmax<int_t>));
            REQUIRE(key_is_compatible(p, symbol_set{"a"}));
            p._set_value(mmp_arr[2][0]);
            REQUIRE(key_is_compatible(p, symbol_set{"a", "b", "c"}));
            p._set_value(mmp_arr[2][1]);
            REQUIRE(key_is_compatible(p, symbol_set{"a", "b", "c"}));

            // Try to go out of the limits, if possible.
            if (mmp_arr[2][0] > std::get<0>(detail::limits_minmax<int_t>)) {
                p._set_value(mmp_arr[2][0] - int_t(1));
                REQUIRE(!key_is_compatible(p, symbol_set{"a", "b", "c"}));
            }
            if (mmp_arr[2][1] < std::get<1>(detail::limits_minmax<int_t>)) {
                p._set_value(mmp_arr[2][1] + int_t(1));
                REQUIRE(!key_is_compatible(p, symbol_set{"a", "b", "c"}));
            }
        } else {
            const auto &umax_arr = detail::ubp_get_max<int_t>();

            symbol_set s;
            for (decltype(umax_arr.size()) i = 0; i < umax_arr.size(); ++i) {
                s.insert("sym_" + std::to_string(i));
            }
            REQUIRE(key_is_compatible(pm_t{}, s));
            s.insert("x");
            REQUIRE(!key_is_compatible(pm_t{}, s));

            pm_t p;
            p._set_value(std::get<0>(detail::limits_minmax<int_t>));
            REQUIRE(key_is_compatible(p, symbol_set{"a"}));
            p._set_value(std::get<1>(detail::limits_minmax<int_t>));
            REQUIRE(key_is_compatible(p, symbol_set{"a"}));
            p._set_value(umax_arr[2]);
            REQUIRE(key_is_compatible(p, symbol_set{"a", "b", "c"}));
            if (umax_arr[2] < std::get<1>(detail::limits_minmax<int_t>)) {
                p._set_value(umax_arr[2] + int_t(1));
                REQUIRE(!key_is_compatible(p, symbol_set{"a", "b", "c"}));
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
