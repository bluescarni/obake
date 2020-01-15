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
#include <obake/math/integrate.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("integrate_test")
{
    REQUIRE(!is_integrable_v<void>);
    REQUIRE(!is_integrable_v<const void>);

    REQUIRE(!is_integrable_v<std::string>);
    REQUIRE(!is_integrable_v<std::string &>);
    REQUIRE(!is_integrable_v<const std::string &>);
    REQUIRE(!is_integrable_v<const std::string>);

    REQUIRE(!is_integrable_v<int>);
    REQUIRE(!is_integrable_v<int &>);
    REQUIRE(!is_integrable_v<const int>);
    REQUIRE(!is_integrable_v<const int &>);

    REQUIRE(!is_integrable_v<double>);
    REQUIRE(!is_integrable_v<double &>);
    REQUIRE(!is_integrable_v<const double>);
    REQUIRE(!is_integrable_v<const double &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Integrable<void>);
    REQUIRE(!Integrable<const void>);

    REQUIRE(!Integrable<std::string>);
    REQUIRE(!Integrable<std::string &>);
    REQUIRE(!Integrable<const std::string &>);
    REQUIRE(!Integrable<const std::string>);

    REQUIRE(!Integrable<int>);
    REQUIRE(!Integrable<int &>);
    REQUIRE(!Integrable<const int>);
    REQUIRE(!Integrable<const int &>);

    REQUIRE(!Integrable<double>);
    REQUIRE(!Integrable<double &>);
    REQUIRE(!Integrable<const double>);
    REQUIRE(!Integrable<const double &>);
#endif
}

struct nointegrate_00 {
};

namespace ns
{

struct integrate_00 {
};

integrate_00 integrate(const integrate_00 &, const std::string &);

struct integrate_01 {
};

integrate_01 integrate(integrate_01 &, const std::string &);

struct integrate_02 {
};

// Wrong signature.
void integrate(const integrate_02 &);

} // namespace ns

struct integrate_ext {
};

struct nointegrate_ext_00 {
};

struct nointegrate_ext_01 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, integrate_ext> inline constexpr auto integrate<T>
#else
inline constexpr auto integrate<T, std::enable_if_t<is_same_cvr_v<T, integrate_ext>>>
#endif
    = [](auto &&, const std::string &) constexpr noexcept
{
    return integrate_ext{};
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, nointegrate_ext_00> inline constexpr auto integrate<T>
#else
inline constexpr auto integrate<T, std::enable_if_t<is_same_cvr_v<T, nointegrate_ext_00>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return 1;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, nointegrate_ext_01> inline constexpr auto integrate<T>
#else
inline constexpr auto integrate<T, std::enable_if_t<is_same_cvr_v<T, nointegrate_ext_01>>>
#endif
    = [](nointegrate_ext_01 &, const std::string &) constexpr noexcept
{
    return nointegrate_ext_01{};
};

} // namespace obake::customisation

TEST_CASE("integrate_custom_test")
{
    REQUIRE(is_integrable_v<integrate_ext>);
    REQUIRE(is_integrable_v<integrate_ext &>);
    REQUIRE(is_integrable_v<const integrate_ext &>);
    REQUIRE(is_integrable_v<const integrate_ext>);

    REQUIRE(!is_integrable_v<nointegrate_ext_00>);
    REQUIRE(!is_integrable_v<nointegrate_ext_00 &>);
    REQUIRE(!is_integrable_v<const nointegrate_ext_00 &>);
    REQUIRE(!is_integrable_v<const nointegrate_ext_00>);

    REQUIRE(!is_integrable_v<nointegrate_ext_01>);
    REQUIRE(is_integrable_v<nointegrate_ext_01 &>);
    REQUIRE(!is_integrable_v<const nointegrate_ext_01 &>);
    REQUIRE(!is_integrable_v<const nointegrate_ext_01>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(Integrable<integrate_ext>);
    REQUIRE(Integrable<integrate_ext &>);
    REQUIRE(Integrable<const integrate_ext &>);
    REQUIRE(Integrable<const integrate_ext>);

    REQUIRE(!Integrable<nointegrate_ext_00>);
    REQUIRE(!Integrable<nointegrate_ext_00 &>);
    REQUIRE(!Integrable<const nointegrate_ext_00 &>);
    REQUIRE(!Integrable<const nointegrate_ext_00>);

    REQUIRE(!Integrable<nointegrate_ext_01>);
    REQUIRE(Integrable<nointegrate_ext_01 &>);
    REQUIRE(!Integrable<const nointegrate_ext_01 &>);
    REQUIRE(!Integrable<const nointegrate_ext_01>);
#endif
}
