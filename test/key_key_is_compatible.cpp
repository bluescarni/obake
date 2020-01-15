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
#include <obake/key/key_is_compatible.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

// ADL-based implementations.
namespace ns
{

struct ct00 {
};

bool key_is_compatible(const ct00 &, const symbol_set &);

struct ct01 {
};

// Disable certain overloads.
bool key_is_compatible(ct01 &, const symbol_set &);

struct nct00 {
};

// Wrong return type.
std::string key_is_compatible(nct00 &, const symbol_set &);

} // namespace ns

struct ext_ct00 {
};

struct ext_ct01 {
};

struct ext_nct00 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_ct00> inline constexpr auto key_is_compatible<T>
#else
inline constexpr auto key_is_compatible<T, std::enable_if_t<is_same_cvr_v<T, ext_ct00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_ct01> inline constexpr auto key_is_compatible<T>
#else
inline constexpr auto key_is_compatible<T, std::enable_if_t<is_same_cvr_v<T, ext_ct01>>>
#endif
    = [](auto &, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_nct00> inline constexpr auto key_is_compatible<T>
#else
inline constexpr auto key_is_compatible<T, std::enable_if_t<is_same_cvr_v<T, ext_nct00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return std::string{};
};

} // namespace obake::customisation

TEST_CASE("key_is_compatible_test")
{
    REQUIRE(!is_compatibility_testable_key_v<void>);

    REQUIRE(!is_compatibility_testable_key_v<int>);
    REQUIRE(!is_compatibility_testable_key_v<int &>);
    REQUIRE(!is_compatibility_testable_key_v<const int &>);
    REQUIRE(!is_compatibility_testable_key_v<int &&>);

    REQUIRE(!is_compatibility_testable_key_v<std::string>);
    REQUIRE(!is_compatibility_testable_key_v<std::string &>);
    REQUIRE(!is_compatibility_testable_key_v<const std::string &>);
    REQUIRE(!is_compatibility_testable_key_v<std::string &&>);

    REQUIRE(is_compatibility_testable_key_v<ns::ct00>);
    REQUIRE(is_compatibility_testable_key_v<ns::ct00 &>);
    REQUIRE(is_compatibility_testable_key_v<const ns::ct00 &>);
    REQUIRE(is_compatibility_testable_key_v<ns::ct00 &&>);

    REQUIRE(!is_compatibility_testable_key_v<ns::ct01>);
    REQUIRE(is_compatibility_testable_key_v<ns::ct01 &>);
    REQUIRE(!is_compatibility_testable_key_v<const ns::ct01 &>);
    REQUIRE(!is_compatibility_testable_key_v<ns::ct01 &&>);

    REQUIRE(!is_compatibility_testable_key_v<const ns::nct00 &>);

    REQUIRE(is_compatibility_testable_key_v<ext_ct00>);
    REQUIRE(is_compatibility_testable_key_v<ext_ct00 &>);
    REQUIRE(is_compatibility_testable_key_v<const ext_ct00 &>);
    REQUIRE(is_compatibility_testable_key_v<ext_ct00 &&>);

    REQUIRE(!is_compatibility_testable_key_v<ext_ct01>);
    REQUIRE(is_compatibility_testable_key_v<ext_ct01 &>);
    REQUIRE(is_compatibility_testable_key_v<const ext_ct01 &>);
    REQUIRE(!is_compatibility_testable_key_v<ext_ct01 &&>);

    REQUIRE(!is_compatibility_testable_key_v<const ext_nct00 &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!CompatibilityTestableKey<void>);

    REQUIRE(!CompatibilityTestableKey<int>);
    REQUIRE(!CompatibilityTestableKey<int &>);
    REQUIRE(!CompatibilityTestableKey<const int &>);
    REQUIRE(!CompatibilityTestableKey<int &&>);

    REQUIRE(!CompatibilityTestableKey<std::string>);
    REQUIRE(!CompatibilityTestableKey<std::string &>);
    REQUIRE(!CompatibilityTestableKey<const std::string &>);
    REQUIRE(!CompatibilityTestableKey<std::string &&>);

    REQUIRE(CompatibilityTestableKey<ns::ct00>);
    REQUIRE(CompatibilityTestableKey<ns::ct00 &>);
    REQUIRE(CompatibilityTestableKey<const ns::ct00 &>);
    REQUIRE(CompatibilityTestableKey<ns::ct00 &&>);

    REQUIRE(!CompatibilityTestableKey<ns::ct01>);
    REQUIRE(CompatibilityTestableKey<ns::ct01 &>);
    REQUIRE(!CompatibilityTestableKey<const ns::ct01 &>);
    REQUIRE(!CompatibilityTestableKey<ns::ct01 &&>);

    REQUIRE(!CompatibilityTestableKey<const ns::nct00 &>);

    REQUIRE(CompatibilityTestableKey<ext_ct00>);
    REQUIRE(CompatibilityTestableKey<ext_ct00 &>);
    REQUIRE(CompatibilityTestableKey<const ext_ct00 &>);
    REQUIRE(CompatibilityTestableKey<ext_ct00 &&>);

    REQUIRE(!CompatibilityTestableKey<ext_ct01>);
    REQUIRE(CompatibilityTestableKey<ext_ct01 &>);
    REQUIRE(CompatibilityTestableKey<const ext_ct01 &>);
    REQUIRE(!CompatibilityTestableKey<ext_ct01 &&>);

    REQUIRE(!CompatibilityTestableKey<const ext_nct00 &>);
#endif
}
