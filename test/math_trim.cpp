// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>
#include <type_traits>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <obake/config.hpp>
#include <obake/math/trim.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

struct notrim_00 {
};

namespace ns
{

struct trim_00 {
};

trim_00 trim(const trim_00 &);

struct trim_01 {
};

trim_01 trim(trim_01 &);

struct trim_02 {
};

// Wrong return type.
void trim(const trim_02 &);

struct trim_03 {
};

// Wrong return type.
int trim(const trim_03 &);

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
requires SameCvr<T, trim_ext> inline constexpr auto trim<T>
#else
inline constexpr auto trim<T, std::enable_if_t<is_same_cvr_v<T, trim_ext>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return trim_ext{};
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, notrim_ext_00> inline constexpr auto trim<T>
#else
inline constexpr auto trim<T, std::enable_if_t<is_same_cvr_v<T, notrim_ext_00>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return 1;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, notrim_ext_01> inline constexpr auto trim<T>
#else
inline constexpr auto trim<T, std::enable_if_t<is_same_cvr_v<T, notrim_ext_01>>>
#endif
    = [](notrim_ext_01 &) constexpr noexcept
{
    return notrim_ext_01{};
};

} // namespace obake::customisation

TEST_CASE("trim_test")
{
    REQUIRE(!is_trimmable_v<void>);

    REQUIRE(is_trimmable_v<int>);
    REQUIRE(is_trimmable_v<int &>);
    REQUIRE(is_trimmable_v<const int &>);
    REQUIRE(is_trimmable_v<int &&>);

    REQUIRE(is_trimmable_v<std::string>);
    REQUIRE(is_trimmable_v<std::string &>);
    REQUIRE(is_trimmable_v<const std::string &>);
    REQUIRE(is_trimmable_v<std::string &&>);

    REQUIRE(is_trimmable_v<mppp::integer<1>>);
    REQUIRE(is_trimmable_v<mppp::rational<1>>);

    REQUIRE(is_trimmable_v<notrim_00>);
    REQUIRE(is_trimmable_v<notrim_00 &>);
    REQUIRE(is_trimmable_v<const notrim_00 &>);
    REQUIRE(is_trimmable_v<notrim_00 &&>);

    REQUIRE(is_trimmable_v<ns::trim_00>);
    REQUIRE(is_trimmable_v<ns::trim_00 &>);
    REQUIRE(is_trimmable_v<const ns::trim_00 &>);
    REQUIRE(is_trimmable_v<ns::trim_00 &&>);

    REQUIRE(is_trimmable_v<ns::trim_01>);
    REQUIRE(is_trimmable_v<ns::trim_01 &>);
    REQUIRE(is_trimmable_v<const ns::trim_01 &>);
    REQUIRE(is_trimmable_v<ns::trim_01 &&>);

    REQUIRE(!is_trimmable_v<ns::trim_02>);
    REQUIRE(!is_trimmable_v<ns::trim_02 &>);
    REQUIRE(!is_trimmable_v<ns::trim_02 &>);
    REQUIRE(!is_trimmable_v<ns::trim_02 &&>);

    REQUIRE(!is_trimmable_v<ns::trim_03>);
    REQUIRE(!is_trimmable_v<ns::trim_03 &>);
    REQUIRE(!is_trimmable_v<ns::trim_03 &>);
    REQUIRE(!is_trimmable_v<ns::trim_03 &&>);

    REQUIRE(is_trimmable_v<trim_ext>);
    REQUIRE(is_trimmable_v<trim_ext &>);
    REQUIRE(is_trimmable_v<const trim_ext &>);
    REQUIRE(is_trimmable_v<trim_ext &&>);

    REQUIRE(!is_trimmable_v<notrim_ext_00>);
    REQUIRE(!is_trimmable_v<notrim_ext_00 &>);
    REQUIRE(!is_trimmable_v<const notrim_ext_00 &>);
    REQUIRE(!is_trimmable_v<notrim_ext_00 &&>);

    REQUIRE(is_trimmable_v<notrim_ext_01>);
    REQUIRE(is_trimmable_v<notrim_ext_01 &>);
    REQUIRE(is_trimmable_v<const notrim_ext_01 &>);
    REQUIRE(is_trimmable_v<notrim_ext_01 &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Trimmable<void>);

    REQUIRE(Trimmable<int>);
    REQUIRE(Trimmable<int &>);
    REQUIRE(Trimmable<const int &>);
    REQUIRE(Trimmable<int &&>);

    REQUIRE(Trimmable<std::string>);
    REQUIRE(Trimmable<std::string &>);
    REQUIRE(Trimmable<const std::string &>);
    REQUIRE(Trimmable<std::string &&>);

    REQUIRE(Trimmable<mppp::integer<1>>);
    REQUIRE(Trimmable<mppp::rational<1>>);

    REQUIRE(Trimmable<notrim_00>);
    REQUIRE(Trimmable<notrim_00 &>);
    REQUIRE(Trimmable<const notrim_00 &>);
    REQUIRE(Trimmable<notrim_00 &&>);

    REQUIRE(Trimmable<ns::trim_00>);
    REQUIRE(Trimmable<ns::trim_00 &>);
    REQUIRE(Trimmable<const ns::trim_00 &>);
    REQUIRE(Trimmable<ns::trim_00 &&>);

    REQUIRE(Trimmable<ns::trim_01>);
    REQUIRE(Trimmable<ns::trim_01 &>);
    REQUIRE(Trimmable<const ns::trim_01 &>);
    REQUIRE(Trimmable<ns::trim_01 &&>);

    REQUIRE(!Trimmable<ns::trim_02>);
    REQUIRE(!Trimmable<ns::trim_02 &>);
    REQUIRE(!Trimmable<ns::trim_02 &>);
    REQUIRE(!Trimmable<ns::trim_02 &&>);

    REQUIRE(!Trimmable<ns::trim_03>);
    REQUIRE(!Trimmable<ns::trim_03 &>);
    REQUIRE(!Trimmable<ns::trim_03 &>);
    REQUIRE(!Trimmable<ns::trim_03 &&>);

    REQUIRE(Trimmable<trim_ext>);
    REQUIRE(Trimmable<trim_ext &>);
    REQUIRE(Trimmable<const trim_ext &>);
    REQUIRE(Trimmable<trim_ext &&>);

    REQUIRE(!Trimmable<notrim_ext_00>);
    REQUIRE(!Trimmable<notrim_ext_00 &>);
    REQUIRE(!Trimmable<const notrim_ext_00 &>);
    REQUIRE(!Trimmable<notrim_ext_00 &&>);

    REQUIRE(Trimmable<notrim_ext_01>);
    REQUIRE(Trimmable<notrim_ext_01 &>);
    REQUIRE(Trimmable<const notrim_ext_01 &>);
    REQUIRE(Trimmable<notrim_ext_01 &&>);
#endif
}
