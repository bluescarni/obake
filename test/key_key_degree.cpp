// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>
#include <type_traits>

#include <obake/config.hpp>
#include <obake/key/key_degree.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

// ADL-based implementations.
namespace ns
{

struct zt00 {
};

bool key_degree(const zt00 &, const symbol_set &);

struct zt01 {
};

// Disable certain overloads.
bool key_degree(zt01 &, const symbol_set &);

} // namespace ns

struct ext_zt00 {
};

struct ext_zt01 {
};

struct ext_nzt00 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_zt00> inline constexpr auto key_degree<T>
#else
inline constexpr auto key_degree<T, std::enable_if_t<is_same_cvr_v<T, ext_zt00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_zt01> inline constexpr auto key_degree<T>
#else
inline constexpr auto key_degree<T, std::enable_if_t<is_same_cvr_v<T, ext_zt01>>>
#endif
    = [](auto &, const symbol_set &) constexpr noexcept
{
    return true;
};

} // namespace obake::customisation

TEST_CASE("key_degree_test")
{
    REQUIRE(!is_key_with_degree_v<void>);

    REQUIRE(!is_key_with_degree_v<int>);
    REQUIRE(!is_key_with_degree_v<int &>);
    REQUIRE(!is_key_with_degree_v<const int &>);
    REQUIRE(!is_key_with_degree_v<int &&>);

    REQUIRE(!is_key_with_degree_v<std::string>);
    REQUIRE(!is_key_with_degree_v<std::string &>);
    REQUIRE(!is_key_with_degree_v<const std::string &>);
    REQUIRE(!is_key_with_degree_v<std::string &&>);

    REQUIRE(is_key_with_degree_v<ns::zt00>);
    REQUIRE(is_key_with_degree_v<ns::zt00 &>);
    REQUIRE(is_key_with_degree_v<const ns::zt00 &>);
    REQUIRE(is_key_with_degree_v<ns::zt00 &&>);

    REQUIRE(!is_key_with_degree_v<ns::zt01>);
    REQUIRE(is_key_with_degree_v<ns::zt01 &>);
    REQUIRE(!is_key_with_degree_v<const ns::zt01 &>);
    REQUIRE(!is_key_with_degree_v<ns::zt01 &&>);

    REQUIRE(is_key_with_degree_v<ext_zt00>);
    REQUIRE(is_key_with_degree_v<ext_zt00 &>);
    REQUIRE(is_key_with_degree_v<const ext_zt00 &>);
    REQUIRE(is_key_with_degree_v<ext_zt00 &&>);

    REQUIRE(!is_key_with_degree_v<ext_zt01>);
    REQUIRE(is_key_with_degree_v<ext_zt01 &>);
    REQUIRE(is_key_with_degree_v<const ext_zt01 &>);
    REQUIRE(!is_key_with_degree_v<ext_zt01 &&>);

    REQUIRE(!is_key_with_degree_v<const ext_nzt00 &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!KeyWithDegree<void>);

    REQUIRE(!KeyWithDegree<int>);
    REQUIRE(!KeyWithDegree<int &>);
    REQUIRE(!KeyWithDegree<const int &>);
    REQUIRE(!KeyWithDegree<int &&>);

    REQUIRE(!KeyWithDegree<std::string>);
    REQUIRE(!KeyWithDegree<std::string &>);
    REQUIRE(!KeyWithDegree<const std::string &>);
    REQUIRE(!KeyWithDegree<std::string &&>);

    REQUIRE(KeyWithDegree<ns::zt00>);
    REQUIRE(KeyWithDegree<ns::zt00 &>);
    REQUIRE(KeyWithDegree<const ns::zt00 &>);
    REQUIRE(KeyWithDegree<ns::zt00 &&>);

    REQUIRE(!KeyWithDegree<ns::zt01>);
    REQUIRE(KeyWithDegree<ns::zt01 &>);
    REQUIRE(!KeyWithDegree<const ns::zt01 &>);
    REQUIRE(!KeyWithDegree<ns::zt01 &&>);

    REQUIRE(KeyWithDegree<ext_zt00>);
    REQUIRE(KeyWithDegree<ext_zt00 &>);
    REQUIRE(KeyWithDegree<const ext_zt00 &>);
    REQUIRE(KeyWithDegree<ext_zt00 &&>);

    REQUIRE(!KeyWithDegree<ext_zt01>);
    REQUIRE(KeyWithDegree<ext_zt01 &>);
    REQUIRE(KeyWithDegree<const ext_zt01 &>);
    REQUIRE(!KeyWithDegree<ext_zt01 &&>);

    REQUIRE(!KeyWithDegree<const ext_nzt00 &>);
#endif
}
