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

#include <bitset>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <piranha/config.hpp>
#include <piranha/detail/tuple_for_each.hpp>
#include <piranha/hash.hpp>
#include <piranha/key/key_is_zero.hpp>
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

TEST_CASE("ctor_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using bp_t = bit_packer<int_t>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_semi_regular_v<pm_t>);

        pm_t pm0;
        // Default ctor.
        REQUIRE(pm0.get_value() == int_t(0));

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

        // Test some type traits.
        REQUIRE(!std::is_constructible_v<pm_t, void>);
        REQUIRE(!std::is_constructible_v<pm_t, int>);
        REQUIRE(std::is_constructible_v<pm_t, std::istream_iterator<char>, unsigned>);
        REQUIRE(!std::is_constructible_v<pm_t, int, unsigned>);
        REQUIRE(!std::is_constructible_v<pm_t, std::istream_iterator<char>, std::istream_iterator<char>>);
        REQUIRE(!std::is_constructible_v<pm_t, std::initializer_list<std::string>>);
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

        constexpr auto width = std::numeric_limits<std::size_t>::digits;

        std::cout << std::bitset<width>(hash(pm_t{1, 2, 3})) << '\n';
        std::cout << std::bitset<width>(hash(pm_t{4, 5, 6})) << '\n';
    });
}
