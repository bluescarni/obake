// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>
#include <type_traits>
#include <vector>

#include <obake/config.hpp>
#include <obake/key/key_trim_identify.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

// ADL-based implementations.
namespace ns
{

struct zt00 {
};

void key_trim_identify(std::vector<int> &, const zt00 &, const symbol_set &);

struct zt01 {
};

// Disable certain overloads.
int key_trim_identify(std::vector<int> &, zt01 &, const symbol_set &);

struct nzt00 {
};

// Wrong signature.
std::string key_trim_identify(nzt00 &, const symbol_set &);

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
requires SameCvr<T, ext_zt00> inline constexpr auto key_trim_identify<T>
#else
inline constexpr auto key_trim_identify<T, std::enable_if_t<is_same_cvr_v<T, ext_zt00>>>
#endif
    = [](std::vector<int> &, auto &&, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_zt01> inline constexpr auto key_trim_identify<T>
#else
inline constexpr auto key_trim_identify<T, std::enable_if_t<is_same_cvr_v<T, ext_zt01>>>
#endif
    = [](std::vector<int> &, auto &, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_nzt00> inline constexpr auto key_trim_identify<T>
#else
inline constexpr auto key_trim_identify<T, std::enable_if_t<is_same_cvr_v<T, ext_nzt00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return std::string{};
};

} // namespace obake::customisation

TEST_CASE("key_trim_identify_test")
{
    REQUIRE(!is_trim_identifiable_key_v<void>);

    REQUIRE(!is_trim_identifiable_key_v<int>);
    REQUIRE(!is_trim_identifiable_key_v<int &>);
    REQUIRE(!is_trim_identifiable_key_v<const int &>);
    REQUIRE(!is_trim_identifiable_key_v<int &&>);

    REQUIRE(!is_trim_identifiable_key_v<std::string>);
    REQUIRE(!is_trim_identifiable_key_v<std::string &>);
    REQUIRE(!is_trim_identifiable_key_v<const std::string &>);
    REQUIRE(!is_trim_identifiable_key_v<std::string &&>);

    REQUIRE(is_trim_identifiable_key_v<ns::zt00>);
    REQUIRE(is_trim_identifiable_key_v<ns::zt00 &>);
    REQUIRE(is_trim_identifiable_key_v<const ns::zt00 &>);
    REQUIRE(is_trim_identifiable_key_v<ns::zt00 &&>);

    REQUIRE(!is_trim_identifiable_key_v<ns::zt01>);
    REQUIRE(is_trim_identifiable_key_v<ns::zt01 &>);
    REQUIRE(!is_trim_identifiable_key_v<const ns::zt01 &>);
    REQUIRE(!is_trim_identifiable_key_v<ns::zt01 &&>);
    // Ensure the return type is cast to void.
    std::vector<int> v;
    ns::zt01 z0;
    REQUIRE(std::is_same_v<void, decltype(key_trim_identify(v, z0, symbol_set{}))>);

    REQUIRE(!is_trim_identifiable_key_v<const ns::nzt00 &>);

    REQUIRE(is_trim_identifiable_key_v<ext_zt00>);
    REQUIRE(is_trim_identifiable_key_v<ext_zt00 &>);
    REQUIRE(is_trim_identifiable_key_v<const ext_zt00 &>);
    REQUIRE(is_trim_identifiable_key_v<ext_zt00 &&>);
    ext_zt00 z1;
    REQUIRE(std::is_same_v<void, decltype(key_trim_identify(v, z1, symbol_set{}))>);

    REQUIRE(!is_trim_identifiable_key_v<ext_zt01>);
    REQUIRE(is_trim_identifiable_key_v<ext_zt01 &>);
    REQUIRE(is_trim_identifiable_key_v<const ext_zt01 &>);
    REQUIRE(!is_trim_identifiable_key_v<ext_zt01 &&>);
    ext_zt00 z2;
    REQUIRE(std::is_same_v<void, decltype(key_trim_identify(v, z2, symbol_set{}))>);

    REQUIRE(!is_trim_identifiable_key_v<const ext_nzt00 &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!TrimIdentifiableKey<void>);

    REQUIRE(!TrimIdentifiableKey<int>);
    REQUIRE(!TrimIdentifiableKey<int &>);
    REQUIRE(!TrimIdentifiableKey<const int &>);
    REQUIRE(!TrimIdentifiableKey<int &&>);

    REQUIRE(!TrimIdentifiableKey<std::string>);
    REQUIRE(!TrimIdentifiableKey<std::string &>);
    REQUIRE(!TrimIdentifiableKey<const std::string &>);
    REQUIRE(!TrimIdentifiableKey<std::string &&>);

    REQUIRE(TrimIdentifiableKey<ns::zt00>);
    REQUIRE(TrimIdentifiableKey<ns::zt00 &>);
    REQUIRE(TrimIdentifiableKey<const ns::zt00 &>);
    REQUIRE(TrimIdentifiableKey<ns::zt00 &&>);

    REQUIRE(!TrimIdentifiableKey<ns::zt01>);
    REQUIRE(TrimIdentifiableKey<ns::zt01 &>);
    REQUIRE(!TrimIdentifiableKey<const ns::zt01 &>);
    REQUIRE(!TrimIdentifiableKey<ns::zt01 &&>);

    REQUIRE(!TrimIdentifiableKey<const ns::nzt00 &>);

    REQUIRE(TrimIdentifiableKey<ext_zt00>);
    REQUIRE(TrimIdentifiableKey<ext_zt00 &>);
    REQUIRE(TrimIdentifiableKey<const ext_zt00 &>);
    REQUIRE(TrimIdentifiableKey<ext_zt00 &&>);

    REQUIRE(!TrimIdentifiableKey<ext_zt01>);
    REQUIRE(TrimIdentifiableKey<ext_zt01 &>);
    REQUIRE(TrimIdentifiableKey<const ext_zt01 &>);
    REQUIRE(!TrimIdentifiableKey<ext_zt01 &&>);

    REQUIRE(!TrimIdentifiableKey<const ext_nzt00 &>);
#endif
}
