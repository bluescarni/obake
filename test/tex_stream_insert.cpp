// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>

#include <mp++/rational.hpp>

#include <obake/config.hpp>
#include <obake/tex_stream_insert.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

// ADL-based implementations.
namespace ns
{

struct si00 {
};

void tex_stream_insert(std::ostream &os, const si00 &);

struct si01 {
};

// Disable certain overloads.
void tex_stream_insert(std::ostream &os, si01 &);

struct nsi00 {
};

} // namespace ns

struct ext_si00 {
};

struct ext_si01 {
};

struct int_si00 {
};

struct int_si01 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_si00> inline constexpr auto tex_stream_insert<T>
#else
inline constexpr auto tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ext_si00>>>
#endif
    = [](std::ostream &, auto &&) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_si01> inline constexpr auto tex_stream_insert<T>
#else
inline constexpr auto tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ext_si01>>>
#endif
    = [](std::ostream &, auto &) constexpr noexcept
{
    return true;
};

namespace internal
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, int_si00> inline constexpr auto tex_stream_insert<T>
#else
inline constexpr auto tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, int_si00>>>
#endif
    = [](std::ostream &, auto &&) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, int_si01> inline constexpr auto tex_stream_insert<T>
#else
inline constexpr auto tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, int_si01>>>
#endif
    = [](std::ostream &, auto &) constexpr noexcept
{
    return true;
};

} // namespace internal

} // namespace obake::customisation

TEST_CASE("tex_stream_insert_test")
{
    REQUIRE(!is_tex_stream_insertable_v<void>);

    REQUIRE(is_tex_stream_insertable_v<int>);
    REQUIRE(is_tex_stream_insertable_v<int &>);
    REQUIRE(is_tex_stream_insertable_v<const int &>);
    REQUIRE(is_tex_stream_insertable_v<std::string>);
    REQUIRE(is_tex_stream_insertable_v<std::string &>);
    REQUIRE(is_tex_stream_insertable_v<const std::string &>);

    // Verify that tex_stream_insert() default
    // to normal stream insertion.
    std::ostringstream oss1, oss2;
    tex_stream_insert(oss1, 42);
    oss2 << 42;
    REQUIRE(oss1.str() == oss2.str());

    REQUIRE(is_tex_stream_insertable_v<ns::si00>);
    REQUIRE(is_tex_stream_insertable_v<ns::si00 &>);
    REQUIRE(is_tex_stream_insertable_v<const ns::si00 &>);
    REQUIRE(is_tex_stream_insertable_v<ns::si00 &&>);

    REQUIRE(!is_tex_stream_insertable_v<ns::si01>);
    REQUIRE(is_tex_stream_insertable_v<ns::si01 &>);
    REQUIRE(!is_tex_stream_insertable_v<const ns::si01 &>);
    REQUIRE(!is_tex_stream_insertable_v<ns::si01 &&>);

    REQUIRE(!is_tex_stream_insertable_v<ns::nsi00>);
    REQUIRE(!is_tex_stream_insertable_v<ns::nsi00 &>);
    REQUIRE(!is_tex_stream_insertable_v<const ns::nsi00 &>);
    REQUIRE(!is_tex_stream_insertable_v<ns::nsi00 &&>);

    REQUIRE(is_tex_stream_insertable_v<ext_si00>);
    REQUIRE(is_tex_stream_insertable_v<ext_si00 &>);
    REQUIRE(is_tex_stream_insertable_v<const ext_si00 &>);
    REQUIRE(is_tex_stream_insertable_v<ext_si00 &&>);

    REQUIRE(!is_tex_stream_insertable_v<ext_si01>);
    REQUIRE(is_tex_stream_insertable_v<ext_si01 &>);
    REQUIRE(is_tex_stream_insertable_v<const ext_si01 &>);
    REQUIRE(!is_tex_stream_insertable_v<ext_si01 &&>);

    REQUIRE(is_tex_stream_insertable_v<int_si00>);
    REQUIRE(is_tex_stream_insertable_v<int_si00 &>);
    REQUIRE(is_tex_stream_insertable_v<const int_si00 &>);
    REQUIRE(is_tex_stream_insertable_v<int_si00 &&>);

    REQUIRE(!is_tex_stream_insertable_v<int_si01>);
    REQUIRE(is_tex_stream_insertable_v<int_si01 &>);
    REQUIRE(is_tex_stream_insertable_v<const int_si01 &>);
    REQUIRE(!is_tex_stream_insertable_v<int_si01 &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!TexStreamInsertable<void>);

    REQUIRE(TexStreamInsertable<int>);
    REQUIRE(TexStreamInsertable<int &>);
    REQUIRE(TexStreamInsertable<const int &>);
    REQUIRE(TexStreamInsertable<std::string>);
    REQUIRE(TexStreamInsertable<std::string &>);
    REQUIRE(TexStreamInsertable<const std::string &>);

    REQUIRE(TexStreamInsertable<ns::si00>);
    REQUIRE(TexStreamInsertable<ns::si00 &>);
    REQUIRE(TexStreamInsertable<const ns::si00 &>);
    REQUIRE(TexStreamInsertable<ns::si00 &&>);

    REQUIRE(!TexStreamInsertable<ns::si01>);
    REQUIRE(TexStreamInsertable<ns::si01 &>);
    REQUIRE(!TexStreamInsertable<const ns::si01 &>);
    REQUIRE(!TexStreamInsertable<ns::si01 &&>);

    REQUIRE(!TexStreamInsertable<ns::nsi00>);
    REQUIRE(!TexStreamInsertable<ns::nsi00 &>);
    REQUIRE(!TexStreamInsertable<const ns::nsi00 &>);
    REQUIRE(!TexStreamInsertable<ns::nsi00 &&>);

    REQUIRE(TexStreamInsertable<ext_si00>);
    REQUIRE(TexStreamInsertable<ext_si00 &>);
    REQUIRE(TexStreamInsertable<const ext_si00 &>);
    REQUIRE(TexStreamInsertable<ext_si00 &&>);

    REQUIRE(!TexStreamInsertable<ext_si01>);
    REQUIRE(TexStreamInsertable<ext_si01 &>);
    REQUIRE(TexStreamInsertable<const ext_si01 &>);
    REQUIRE(!TexStreamInsertable<ext_si01 &&>);

    REQUIRE(TexStreamInsertable<int_si00>);
    REQUIRE(TexStreamInsertable<int_si00 &>);
    REQUIRE(TexStreamInsertable<const int_si00 &>);
    REQUIRE(TexStreamInsertable<int_si00 &&>);

    REQUIRE(!TexStreamInsertable<int_si01>);
    REQUIRE(TexStreamInsertable<int_si01 &>);
    REQUIRE(TexStreamInsertable<const int_si01 &>);
    REQUIRE(!TexStreamInsertable<int_si01 &&>);
#endif
}

#if defined(OBAKE_HAVE_GCC_INT128)

TEST_CASE("tex_stream_insert_int128_test")
{
    REQUIRE(is_tex_stream_insertable_v<__int128_t>);
    REQUIRE(is_tex_stream_insertable_v<__int128_t &>);
    REQUIRE(is_tex_stream_insertable_v<const __int128_t &>);
    REQUIRE(is_tex_stream_insertable_v<__int128_t &&>);

    REQUIRE(is_tex_stream_insertable_v<__uint128_t>);
    REQUIRE(is_tex_stream_insertable_v<__uint128_t &>);
    REQUIRE(is_tex_stream_insertable_v<const __uint128_t &>);
    REQUIRE(is_tex_stream_insertable_v<__uint128_t &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(TexStreamInsertable<__int128_t>);
    REQUIRE(TexStreamInsertable<__int128_t &>);
    REQUIRE(TexStreamInsertable<const __int128_t &>);
    REQUIRE(TexStreamInsertable<__int128_t &&>);

    REQUIRE(TexStreamInsertable<__uint128_t>);
    REQUIRE(TexStreamInsertable<__uint128_t &>);
    REQUIRE(TexStreamInsertable<const __uint128_t &>);
    REQUIRE(TexStreamInsertable<__uint128_t &&>);
#endif

    std::ostringstream oss;
    tex_stream_insert(oss, __int128_t(-42));
    REQUIRE(oss.str() == "-42");
    oss.str("");
    tex_stream_insert(oss, __uint128_t(42));
    REQUIRE(oss.str() == "42");
}

#endif

TEST_CASE("tex_stream_insert_rational_test")
{
    using rat_t = mppp::rational<1>;

    REQUIRE(is_tex_stream_insertable_v<rat_t>);
    REQUIRE(is_tex_stream_insertable_v<rat_t &>);
    REQUIRE(is_tex_stream_insertable_v<const rat_t &>);
    REQUIRE(is_tex_stream_insertable_v<rat_t &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(TexStreamInsertable<rat_t>);
    REQUIRE(TexStreamInsertable<rat_t &>);
    REQUIRE(TexStreamInsertable<const rat_t &>);
    REQUIRE(TexStreamInsertable<rat_t &&>);
#endif

    std::ostringstream oss;

    tex_stream_insert(oss, rat_t{});
    REQUIRE(oss.str() == "0");
    oss.str("");

    tex_stream_insert(oss, rat_t{42});
    REQUIRE(oss.str() == "42");
    oss.str("");

    tex_stream_insert(oss, rat_t{42, 47});
    REQUIRE(oss.str() == "\\frac{42}{47}");
    oss.str("");

    tex_stream_insert(oss, rat_t{42, -47});
    REQUIRE(oss.str() == "-\\frac{42}{47}");
    oss.str("");

    tex_stream_insert(oss, rat_t{1, -47});
    REQUIRE(oss.str() == "-\\frac{1}{47}");
    oss.str("");

    tex_stream_insert(oss, rat_t{1, 3});
    REQUIRE(oss.str() == "\\frac{1}{3}");
    oss.str("");
}
