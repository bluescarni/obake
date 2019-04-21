// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/key/key_is_zero.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <string>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

// ADL-based implementations.
namespace ns
{

struct zt00 {
};

bool key_is_zero(const zt00 &, const symbol_set &);

struct zt01 {
};

// Disable certain overloads.
bool key_is_zero(zt01 &, const symbol_set &);

struct nzt00 {
};

// Wrong return type.
std::string key_is_zero(nzt00 &, const symbol_set &);

} // namespace ns

struct ext_zt00 {
};

struct ext_zt01 {
};

struct ext_nzt00 {
};

namespace piranha::customisation
{

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvref<T, ext_zt00> inline constexpr auto key_is_zero<T>
#else
inline constexpr auto key_is_zero<T, std::enable_if_t<is_same_cvref_v<T, ext_zt00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvref<T, ext_zt01> inline constexpr auto key_is_zero<T>
#else
inline constexpr auto key_is_zero<T, std::enable_if_t<is_same_cvref_v<T, ext_zt01>>>
#endif
    = [](auto &, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvref<T, ext_nzt00> inline constexpr auto key_is_zero<T>
#else
inline constexpr auto key_is_zero<T, std::enable_if_t<is_same_cvref_v<T, ext_nzt00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return std::string{};
};

} // namespace piranha::customisation

TEST_CASE("key_is_zero_test")
{
    REQUIRE(!is_zero_testable_key_v<void>);

    REQUIRE(!is_zero_testable_key_v<int>);
    REQUIRE(!is_zero_testable_key_v<int &>);
    REQUIRE(!is_zero_testable_key_v<const int &>);
    REQUIRE(!is_zero_testable_key_v<int &&>);

    REQUIRE(!is_zero_testable_key_v<std::string>);
    REQUIRE(!is_zero_testable_key_v<std::string &>);
    REQUIRE(!is_zero_testable_key_v<const std::string &>);
    REQUIRE(!is_zero_testable_key_v<std::string &&>);

    REQUIRE(is_zero_testable_key_v<ns::zt00>);
    REQUIRE(is_zero_testable_key_v<ns::zt00 &>);
    REQUIRE(is_zero_testable_key_v<const ns::zt00 &>);
    REQUIRE(is_zero_testable_key_v<ns::zt00 &&>);

    REQUIRE(!is_zero_testable_key_v<ns::zt01>);
    REQUIRE(is_zero_testable_key_v<ns::zt01 &>);
    REQUIRE(!is_zero_testable_key_v<const ns::zt01 &>);
    REQUIRE(!is_zero_testable_key_v<ns::zt01 &&>);

    REQUIRE(!is_zero_testable_key_v<const ns::nzt00 &>);

    REQUIRE(is_zero_testable_key_v<ext_zt00>);
    REQUIRE(is_zero_testable_key_v<ext_zt00 &>);
    REQUIRE(is_zero_testable_key_v<const ext_zt00 &>);
    REQUIRE(is_zero_testable_key_v<ext_zt00 &&>);

    REQUIRE(!is_zero_testable_key_v<ext_zt01>);
    REQUIRE(is_zero_testable_key_v<ext_zt01 &>);
    REQUIRE(is_zero_testable_key_v<const ext_zt01 &>);
    REQUIRE(!is_zero_testable_key_v<ext_zt01 &&>);

    REQUIRE(!is_zero_testable_key_v<const ext_nzt00 &>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!ZeroTestableKey<void>);

    REQUIRE(!ZeroTestableKey<int>);
    REQUIRE(!ZeroTestableKey<int &>);
    REQUIRE(!ZeroTestableKey<const int &>);
    REQUIRE(!ZeroTestableKey<int &&>);

    REQUIRE(!ZeroTestableKey<std::string>);
    REQUIRE(!ZeroTestableKey<std::string &>);
    REQUIRE(!ZeroTestableKey<const std::string &>);
    REQUIRE(!ZeroTestableKey<std::string &&>);

    REQUIRE(ZeroTestableKey<ns::zt00>);
    REQUIRE(ZeroTestableKey<ns::zt00 &>);
    REQUIRE(ZeroTestableKey<const ns::zt00 &>);
    REQUIRE(ZeroTestableKey<ns::zt00 &&>);

    REQUIRE(!ZeroTestableKey<ns::zt01>);
    REQUIRE(ZeroTestableKey<ns::zt01 &>);
    REQUIRE(!ZeroTestableKey<const ns::zt01 &>);
    REQUIRE(!ZeroTestableKey<ns::zt01 &&>);

    REQUIRE(!ZeroTestableKey<const ns::nzt00 &>);

    REQUIRE(ZeroTestableKey<ext_zt00>);
    REQUIRE(ZeroTestableKey<ext_zt00 &>);
    REQUIRE(ZeroTestableKey<const ext_zt00 &>);
    REQUIRE(ZeroTestableKey<ext_zt00 &&>);

    REQUIRE(!ZeroTestableKey<ext_zt01>);
    REQUIRE(ZeroTestableKey<ext_zt01 &>);
    REQUIRE(ZeroTestableKey<const ext_zt01 &>);
    REQUIRE(!ZeroTestableKey<ext_zt01 &&>);

    REQUIRE(!ZeroTestableKey<const ext_nzt00 &>);
#endif
}
