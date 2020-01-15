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
#include <obake/key/key_is_one.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

// ADL-based implementations.
namespace ns
{

struct zt00 {
};

bool key_is_one(const zt00 &, const symbol_set &);

struct zt01 {
};

// Disable certain overloads.
bool key_is_one(zt01 &, const symbol_set &);

struct nzt00 {
};

// Wrong return type.
std::string key_is_one(nzt00 &, const symbol_set &);

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
requires SameCvr<T, ext_zt00> inline constexpr auto key_is_one<T>
#else
inline constexpr auto key_is_one<T, std::enable_if_t<is_same_cvr_v<T, ext_zt00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_zt01> inline constexpr auto key_is_one<T>
#else
inline constexpr auto key_is_one<T, std::enable_if_t<is_same_cvr_v<T, ext_zt01>>>
#endif
    = [](auto &, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_nzt00> inline constexpr auto key_is_one<T>
#else
inline constexpr auto key_is_one<T, std::enable_if_t<is_same_cvr_v<T, ext_nzt00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return std::string{};
};

} // namespace obake::customisation

TEST_CASE("key_is_one_test")
{
    REQUIRE(!is_one_testable_key_v<void>);

    REQUIRE(!is_one_testable_key_v<int>);
    REQUIRE(!is_one_testable_key_v<int &>);
    REQUIRE(!is_one_testable_key_v<const int &>);
    REQUIRE(!is_one_testable_key_v<int &&>);

    REQUIRE(!is_one_testable_key_v<std::string>);
    REQUIRE(!is_one_testable_key_v<std::string &>);
    REQUIRE(!is_one_testable_key_v<const std::string &>);
    REQUIRE(!is_one_testable_key_v<std::string &&>);

    REQUIRE(is_one_testable_key_v<ns::zt00>);
    REQUIRE(is_one_testable_key_v<ns::zt00 &>);
    REQUIRE(is_one_testable_key_v<const ns::zt00 &>);
    REQUIRE(is_one_testable_key_v<ns::zt00 &&>);

    REQUIRE(!is_one_testable_key_v<ns::zt01>);
    REQUIRE(is_one_testable_key_v<ns::zt01 &>);
    REQUIRE(!is_one_testable_key_v<const ns::zt01 &>);
    REQUIRE(!is_one_testable_key_v<ns::zt01 &&>);

    REQUIRE(!is_one_testable_key_v<const ns::nzt00 &>);

    REQUIRE(is_one_testable_key_v<ext_zt00>);
    REQUIRE(is_one_testable_key_v<ext_zt00 &>);
    REQUIRE(is_one_testable_key_v<const ext_zt00 &>);
    REQUIRE(is_one_testable_key_v<ext_zt00 &&>);

    REQUIRE(!is_one_testable_key_v<ext_zt01>);
    REQUIRE(is_one_testable_key_v<ext_zt01 &>);
    REQUIRE(is_one_testable_key_v<const ext_zt01 &>);
    REQUIRE(!is_one_testable_key_v<ext_zt01 &&>);

    REQUIRE(!is_one_testable_key_v<const ext_nzt00 &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!OneTestableKey<void>);

    REQUIRE(!OneTestableKey<int>);
    REQUIRE(!OneTestableKey<int &>);
    REQUIRE(!OneTestableKey<const int &>);
    REQUIRE(!OneTestableKey<int &&>);

    REQUIRE(!OneTestableKey<std::string>);
    REQUIRE(!OneTestableKey<std::string &>);
    REQUIRE(!OneTestableKey<const std::string &>);
    REQUIRE(!OneTestableKey<std::string &&>);

    REQUIRE(OneTestableKey<ns::zt00>);
    REQUIRE(OneTestableKey<ns::zt00 &>);
    REQUIRE(OneTestableKey<const ns::zt00 &>);
    REQUIRE(OneTestableKey<ns::zt00 &&>);

    REQUIRE(!OneTestableKey<ns::zt01>);
    REQUIRE(OneTestableKey<ns::zt01 &>);
    REQUIRE(!OneTestableKey<const ns::zt01 &>);
    REQUIRE(!OneTestableKey<ns::zt01 &&>);

    REQUIRE(!OneTestableKey<const ns::nzt00 &>);

    REQUIRE(OneTestableKey<ext_zt00>);
    REQUIRE(OneTestableKey<ext_zt00 &>);
    REQUIRE(OneTestableKey<const ext_zt00 &>);
    REQUIRE(OneTestableKey<ext_zt00 &&>);

    REQUIRE(!OneTestableKey<ext_zt01>);
    REQUIRE(OneTestableKey<ext_zt01 &>);
    REQUIRE(OneTestableKey<const ext_zt01 &>);
    REQUIRE(!OneTestableKey<ext_zt01 &&>);

    REQUIRE(!OneTestableKey<const ext_nzt00 &>);
#endif
}
