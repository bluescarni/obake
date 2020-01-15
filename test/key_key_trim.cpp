// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>

#include <obake/config.hpp>
#include <obake/key/key_trim.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

struct notrim_00 {
};

namespace ns
{

struct trim_00 {
};

trim_00 key_trim(const trim_00 &, const symbol_idx_set &, const symbol_set &);

struct trim_01 {
};

trim_01 key_trim(trim_01 &, const symbol_idx_set &, const symbol_set &);

struct trim_02 {
};

// Wrong return type.
void key_trim(const trim_02 &, const symbol_idx_set &, const symbol_set &);

struct trim_03 {
};

// Wrong signature.
int key_trim(const trim_03 &);

} // namespace ns

struct trim_ext {
};

struct notrim_ext_00 {
};

struct notrim_ext_01 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, trim_ext> inline constexpr auto key_trim<T>
#else
inline constexpr auto key_trim<T, std::enable_if_t<is_same_cvr_v<T, trim_ext>>>
#endif
    = [](auto &&, const symbol_idx_set &, const symbol_set &) constexpr noexcept
{
    return trim_ext{};
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, notrim_ext_00> inline constexpr auto key_trim<T>
#else
inline constexpr auto key_trim<T, std::enable_if_t<is_same_cvr_v<T, notrim_ext_00>>>
#endif
    = [](auto &&, const symbol_idx_set &, const symbol_set &) constexpr noexcept
{
    return 1;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, notrim_ext_01> inline constexpr auto key_trim<T>
#else
inline constexpr auto key_trim<T, std::enable_if_t<is_same_cvr_v<T, notrim_ext_01>>>
#endif
    = [](notrim_ext_01 &, const symbol_idx_set &, const symbol_set &) constexpr noexcept
{
    return notrim_ext_01{};
};

} // namespace obake::customisation

TEST_CASE("key_trim_test")
{
    REQUIRE(!is_trimmable_key_v<void>);

    REQUIRE(!is_trimmable_key_v<int>);
    REQUIRE(!is_trimmable_key_v<int &>);
    REQUIRE(!is_trimmable_key_v<const int &>);
    REQUIRE(!is_trimmable_key_v<int &&>);

    REQUIRE(!is_trimmable_key_v<notrim_00>);
    REQUIRE(!is_trimmable_key_v<notrim_00 &>);
    REQUIRE(!is_trimmable_key_v<const notrim_00 &>);
    REQUIRE(!is_trimmable_key_v<notrim_00 &&>);

    REQUIRE(is_trimmable_key_v<ns::trim_00>);
    REQUIRE(is_trimmable_key_v<ns::trim_00 &>);
    REQUIRE(is_trimmable_key_v<const ns::trim_00 &>);
    REQUIRE(is_trimmable_key_v<ns::trim_00 &&>);

    REQUIRE(!is_trimmable_key_v<ns::trim_01>);
    REQUIRE(is_trimmable_key_v<ns::trim_01 &>);
    REQUIRE(!is_trimmable_key_v<const ns::trim_01 &>);
    REQUIRE(!is_trimmable_key_v<ns::trim_01 &&>);

    REQUIRE(!is_trimmable_key_v<ns::trim_02>);
    REQUIRE(!is_trimmable_key_v<ns::trim_02 &>);
    REQUIRE(!is_trimmable_key_v<ns::trim_02 &>);
    REQUIRE(!is_trimmable_key_v<ns::trim_02 &&>);

    REQUIRE(!is_trimmable_key_v<ns::trim_03>);
    REQUIRE(!is_trimmable_key_v<ns::trim_03 &>);
    REQUIRE(!is_trimmable_key_v<ns::trim_03 &>);
    REQUIRE(!is_trimmable_key_v<ns::trim_03 &&>);

    REQUIRE(is_trimmable_key_v<trim_ext>);
    REQUIRE(is_trimmable_key_v<trim_ext &>);
    REQUIRE(is_trimmable_key_v<const trim_ext &>);
    REQUIRE(is_trimmable_key_v<trim_ext &&>);

    REQUIRE(!is_trimmable_key_v<notrim_ext_00>);
    REQUIRE(!is_trimmable_key_v<notrim_ext_00 &>);
    REQUIRE(!is_trimmable_key_v<const notrim_ext_00 &>);
    REQUIRE(!is_trimmable_key_v<notrim_ext_00 &&>);

    REQUIRE(!is_trimmable_key_v<notrim_ext_01>);
    REQUIRE(is_trimmable_key_v<notrim_ext_01 &>);
    REQUIRE(!is_trimmable_key_v<const notrim_ext_01 &>);
    REQUIRE(!is_trimmable_key_v<notrim_ext_01 &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!TrimmableKey<void>);

    REQUIRE(!TrimmableKey<int>);
    REQUIRE(!TrimmableKey<int &>);
    REQUIRE(!TrimmableKey<const int &>);
    REQUIRE(!TrimmableKey<int &&>);

    REQUIRE(!TrimmableKey<notrim_00>);
    REQUIRE(!TrimmableKey<notrim_00 &>);
    REQUIRE(!TrimmableKey<const notrim_00 &>);
    REQUIRE(!TrimmableKey<notrim_00 &&>);

    REQUIRE(TrimmableKey<ns::trim_00>);
    REQUIRE(TrimmableKey<ns::trim_00 &>);
    REQUIRE(TrimmableKey<const ns::trim_00 &>);
    REQUIRE(TrimmableKey<ns::trim_00 &&>);

    REQUIRE(!TrimmableKey<ns::trim_01>);
    REQUIRE(TrimmableKey<ns::trim_01 &>);
    REQUIRE(!TrimmableKey<const ns::trim_01 &>);
    REQUIRE(!TrimmableKey<ns::trim_01 &&>);

    REQUIRE(!TrimmableKey<ns::trim_02>);
    REQUIRE(!TrimmableKey<ns::trim_02 &>);
    REQUIRE(!TrimmableKey<ns::trim_02 &>);
    REQUIRE(!TrimmableKey<ns::trim_02 &&>);

    REQUIRE(!TrimmableKey<ns::trim_03>);
    REQUIRE(!TrimmableKey<ns::trim_03 &>);
    REQUIRE(!TrimmableKey<ns::trim_03 &>);
    REQUIRE(!TrimmableKey<ns::trim_03 &&>);

    REQUIRE(TrimmableKey<trim_ext>);
    REQUIRE(TrimmableKey<trim_ext &>);
    REQUIRE(TrimmableKey<const trim_ext &>);
    REQUIRE(TrimmableKey<trim_ext &&>);

    REQUIRE(!TrimmableKey<notrim_ext_00>);
    REQUIRE(!TrimmableKey<notrim_ext_00 &>);
    REQUIRE(!TrimmableKey<const notrim_ext_00 &>);
    REQUIRE(!TrimmableKey<notrim_ext_00 &&>);

    REQUIRE(!TrimmableKey<notrim_ext_01>);
    REQUIRE(TrimmableKey<notrim_ext_01 &>);
    REQUIRE(!TrimmableKey<const notrim_ext_01 &>);
    REQUIRE(!TrimmableKey<notrim_ext_01 &&>);
#endif
}
