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

#include <obake/cf/cf_tex_stream_insert.hpp>
#include <obake/config.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

// ADL-based implementations.
namespace ns
{

struct si00 {
};

void cf_tex_stream_insert(std::ostream &os, const si00 &);

// An implementation that forwards to tex_stream_insert().
struct si00a {
};

inline void tex_stream_insert(std::ostream &os, const si00a &)
{
    os << "Hello world";
}

struct si01 {
};

// Disable certain overloads.
void cf_tex_stream_insert(std::ostream &os, si01 &);

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
requires SameCvr<T, ext_si00> inline constexpr auto cf_tex_stream_insert<T>
#else
inline constexpr auto cf_tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ext_si00>>>
#endif
    = [](std::ostream &, auto &&) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_si01> inline constexpr auto cf_tex_stream_insert<T>
#else
inline constexpr auto cf_tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ext_si01>>>
#endif
    = [](std::ostream &, auto &) constexpr noexcept
{
    return true;
};

namespace internal
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, int_si00> inline constexpr auto cf_tex_stream_insert<T>
#else
inline constexpr auto cf_tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, int_si00>>>
#endif
    = [](std::ostream &, auto &&) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, int_si01> inline constexpr auto cf_tex_stream_insert<T>
#else
inline constexpr auto cf_tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, int_si01>>>
#endif
    = [](std::ostream &, auto &) constexpr noexcept
{
    return true;
};

} // namespace internal

} // namespace obake::customisation

TEST_CASE("cf_tex_stream_insert_test")
{
    REQUIRE(!is_tex_stream_insertable_cf_v<void>);

    REQUIRE(is_tex_stream_insertable_cf_v<int>);
    REQUIRE(is_tex_stream_insertable_cf_v<int &>);
    REQUIRE(is_tex_stream_insertable_cf_v<const int &>);
    REQUIRE(is_tex_stream_insertable_cf_v<std::string>);
    REQUIRE(is_tex_stream_insertable_cf_v<std::string &>);
    REQUIRE(is_tex_stream_insertable_cf_v<const std::string &>);

    std::ostringstream oss1, oss2;
    cf_tex_stream_insert(oss1, 42);
    oss2 << 42;
    REQUIRE(oss1.str() == oss2.str());

    REQUIRE(is_tex_stream_insertable_cf_v<ns::si00>);
    REQUIRE(is_tex_stream_insertable_cf_v<ns::si00 &>);
    REQUIRE(is_tex_stream_insertable_cf_v<const ns::si00 &>);
    REQUIRE(is_tex_stream_insertable_cf_v<ns::si00 &&>);

    REQUIRE(is_tex_stream_insertable_cf_v<ns::si00a>);
    REQUIRE(is_tex_stream_insertable_cf_v<ns::si00a &>);
    REQUIRE(is_tex_stream_insertable_cf_v<const ns::si00a &>);
    REQUIRE(is_tex_stream_insertable_cf_v<ns::si00a &&>);
    oss1.str("");
    cf_tex_stream_insert(oss1, ns::si00a{});
    REQUIRE(oss1.str() == "Hello world");

    REQUIRE(!is_tex_stream_insertable_cf_v<ns::si01>);
    REQUIRE(is_tex_stream_insertable_cf_v<ns::si01 &>);
    REQUIRE(!is_tex_stream_insertable_cf_v<const ns::si01 &>);
    REQUIRE(!is_tex_stream_insertable_cf_v<ns::si01 &&>);

    REQUIRE(!is_tex_stream_insertable_cf_v<ns::nsi00>);
    REQUIRE(!is_tex_stream_insertable_cf_v<ns::nsi00 &>);
    REQUIRE(!is_tex_stream_insertable_cf_v<const ns::nsi00 &>);
    REQUIRE(!is_tex_stream_insertable_cf_v<ns::nsi00 &&>);

    REQUIRE(is_tex_stream_insertable_cf_v<ext_si00>);
    REQUIRE(is_tex_stream_insertable_cf_v<ext_si00 &>);
    REQUIRE(is_tex_stream_insertable_cf_v<const ext_si00 &>);
    REQUIRE(is_tex_stream_insertable_cf_v<ext_si00 &&>);

    REQUIRE(!is_tex_stream_insertable_cf_v<ext_si01>);
    REQUIRE(is_tex_stream_insertable_cf_v<ext_si01 &>);
    REQUIRE(is_tex_stream_insertable_cf_v<const ext_si01 &>);
    REQUIRE(!is_tex_stream_insertable_cf_v<ext_si01 &&>);

    REQUIRE(is_tex_stream_insertable_cf_v<int_si00>);
    REQUIRE(is_tex_stream_insertable_cf_v<int_si00 &>);
    REQUIRE(is_tex_stream_insertable_cf_v<const int_si00 &>);
    REQUIRE(is_tex_stream_insertable_cf_v<int_si00 &&>);

    REQUIRE(!is_tex_stream_insertable_cf_v<int_si01>);
    REQUIRE(is_tex_stream_insertable_cf_v<int_si01 &>);
    REQUIRE(is_tex_stream_insertable_cf_v<const int_si01 &>);
    REQUIRE(!is_tex_stream_insertable_cf_v<int_si01 &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!TexStreamInsertableCf<void>);

    REQUIRE(TexStreamInsertableCf<int>);
    REQUIRE(TexStreamInsertableCf<int &>);
    REQUIRE(TexStreamInsertableCf<const int &>);
    REQUIRE(TexStreamInsertableCf<std::string>);
    REQUIRE(TexStreamInsertableCf<std::string &>);
    REQUIRE(TexStreamInsertableCf<const std::string &>);

    REQUIRE(TexStreamInsertableCf<ns::si00>);
    REQUIRE(TexStreamInsertableCf<ns::si00 &>);
    REQUIRE(TexStreamInsertableCf<const ns::si00 &>);
    REQUIRE(TexStreamInsertableCf<ns::si00 &&>);

    REQUIRE(TexStreamInsertableCf<ns::si00a>);
    REQUIRE(TexStreamInsertableCf<ns::si00a &>);
    REQUIRE(TexStreamInsertableCf<const ns::si00a &>);
    REQUIRE(TexStreamInsertableCf<ns::si00a &&>);

    REQUIRE(!TexStreamInsertableCf<ns::si01>);
    REQUIRE(TexStreamInsertableCf<ns::si01 &>);
    REQUIRE(!TexStreamInsertableCf<const ns::si01 &>);
    REQUIRE(!TexStreamInsertableCf<ns::si01 &&>);

    REQUIRE(!TexStreamInsertableCf<ns::nsi00>);
    REQUIRE(!TexStreamInsertableCf<ns::nsi00 &>);
    REQUIRE(!TexStreamInsertableCf<const ns::nsi00 &>);
    REQUIRE(!TexStreamInsertableCf<ns::nsi00 &&>);

    REQUIRE(TexStreamInsertableCf<ext_si00>);
    REQUIRE(TexStreamInsertableCf<ext_si00 &>);
    REQUIRE(TexStreamInsertableCf<const ext_si00 &>);
    REQUIRE(TexStreamInsertableCf<ext_si00 &&>);

    REQUIRE(!TexStreamInsertableCf<ext_si01>);
    REQUIRE(TexStreamInsertableCf<ext_si01 &>);
    REQUIRE(TexStreamInsertableCf<const ext_si01 &>);
    REQUIRE(!TexStreamInsertableCf<ext_si01 &&>);

    REQUIRE(TexStreamInsertableCf<int_si00>);
    REQUIRE(TexStreamInsertableCf<int_si00 &>);
    REQUIRE(TexStreamInsertableCf<const int_si00 &>);
    REQUIRE(TexStreamInsertableCf<int_si00 &&>);

    REQUIRE(!TexStreamInsertableCf<int_si01>);
    REQUIRE(TexStreamInsertableCf<int_si01 &>);
    REQUIRE(TexStreamInsertableCf<const int_si01 &>);
    REQUIRE(!TexStreamInsertableCf<int_si01 &&>);
#endif
}
