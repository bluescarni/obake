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

#include <obake/config.hpp>
#include <obake/key/key_tex_stream_insert.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

// ADL-based implementations.
namespace ns
{

struct si00 {
};

void key_tex_stream_insert(std::ostream &os, const si00 &, const symbol_set &);

// A class whose implementation offloads to key_stream_insert().
struct si00a {
};

inline void key_stream_insert(std::ostream &os, const si00a &, const symbol_set &)
{
    os << "Hello world";
}

struct si01 {
};

// Disable certain overloads.
void key_tex_stream_insert(std::ostream &os, si01 &, const symbol_set &);

struct nsi00 {
};

// Wrong ref type on symbol_set.
void key_tex_stream_insert(std::ostream &os, const nsi00 &, symbol_set &);

struct nsi01 {
};

// Wrong ref type on symbol_set, but overriden in the external
// customisation point.
void key_tex_stream_insert(std::ostream &os, const nsi01 &, symbol_set &);

} // namespace ns

struct ext_si00 {
};

struct ext_si01 {
};

struct ext_nsi00 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_si00> inline constexpr auto key_tex_stream_insert<T>
#else
inline constexpr auto key_tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ext_si00>>>
#endif
    = [](std::ostream &, auto &&, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_si01> inline constexpr auto key_tex_stream_insert<T>
#else
inline constexpr auto key_tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ext_si01>>>
#endif
    = [](std::ostream &, auto &, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_nsi00> inline constexpr auto key_tex_stream_insert<T>
#else
inline constexpr auto key_tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ext_nsi00>>>
#endif
    = [](std::ostream &, auto &&, symbol_set &) constexpr noexcept
{
    return std::string{};
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ns::nsi01> inline constexpr auto key_tex_stream_insert<T>
#else
inline constexpr auto key_tex_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ns::nsi01>>>
#endif
    = [](std::ostream &, auto &&, const symbol_set &) constexpr noexcept
{
    return true;
};

} // namespace obake::customisation

TEST_CASE("key_tex_stream_insert_test")
{
    REQUIRE(!is_tex_stream_insertable_key_v<void>);

    REQUIRE(!is_tex_stream_insertable_key_v<int>);
    REQUIRE(!is_tex_stream_insertable_key_v<std::string>);

    REQUIRE(is_tex_stream_insertable_key_v<ns::si00>);
    REQUIRE(is_tex_stream_insertable_key_v<ns::si00 &>);
    REQUIRE(is_tex_stream_insertable_key_v<const ns::si00 &>);
    REQUIRE(is_tex_stream_insertable_key_v<ns::si00 &&>);

    REQUIRE(is_tex_stream_insertable_key_v<ns::si00a>);
    REQUIRE(is_tex_stream_insertable_key_v<ns::si00a &>);
    REQUIRE(is_tex_stream_insertable_key_v<const ns::si00a &>);
    REQUIRE(is_tex_stream_insertable_key_v<ns::si00a &&>);
    // Verify the implementation for si00a really forwards
    // to key_stream_insert().
    std::ostringstream oss;
    key_tex_stream_insert(oss, ns::si00a{}, symbol_set{});
    REQUIRE(oss.str() == "Hello world");

    REQUIRE(!is_tex_stream_insertable_key_v<ns::si01>);
    REQUIRE(is_tex_stream_insertable_key_v<ns::si01 &>);
    REQUIRE(!is_tex_stream_insertable_key_v<const ns::si01 &>);
    REQUIRE(!is_tex_stream_insertable_key_v<ns::si01 &&>);

    REQUIRE(!is_tex_stream_insertable_key_v<ns::nsi00>);
    REQUIRE(!is_tex_stream_insertable_key_v<ns::nsi00 &>);
    REQUIRE(!is_tex_stream_insertable_key_v<const ns::nsi00 &>);
    REQUIRE(!is_tex_stream_insertable_key_v<ns::nsi00 &&>);

    // NOTE: this one has the wrong ADL implementation,
    // but a good implementation in the customisation
    // namespace which overrides it.
    REQUIRE(is_tex_stream_insertable_key_v<ns::nsi01>);
    REQUIRE(is_tex_stream_insertable_key_v<ns::nsi01 &>);
    REQUIRE(is_tex_stream_insertable_key_v<const ns::nsi01 &>);
    REQUIRE(is_tex_stream_insertable_key_v<ns::nsi01 &&>);

    REQUIRE(is_tex_stream_insertable_key_v<ext_si00>);
    REQUIRE(is_tex_stream_insertable_key_v<ext_si00 &>);
    REQUIRE(is_tex_stream_insertable_key_v<const ext_si00 &>);
    REQUIRE(is_tex_stream_insertable_key_v<ext_si00 &&>);

    REQUIRE(!is_tex_stream_insertable_key_v<ext_si01>);
    REQUIRE(is_tex_stream_insertable_key_v<ext_si01 &>);
    REQUIRE(is_tex_stream_insertable_key_v<const ext_si01 &>);
    REQUIRE(!is_tex_stream_insertable_key_v<ext_si01 &&>);

    REQUIRE(!is_tex_stream_insertable_key_v<ext_nsi00>);
    REQUIRE(!is_tex_stream_insertable_key_v<ext_nsi00 &>);
    REQUIRE(!is_tex_stream_insertable_key_v<const ext_nsi00 &>);
    REQUIRE(!is_tex_stream_insertable_key_v<ext_nsi00 &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!TexStreamInsertableKey<void>);

    REQUIRE(!TexStreamInsertableKey<int>);
    REQUIRE(!TexStreamInsertableKey<std::string>);

    REQUIRE(TexStreamInsertableKey<ns::si00>);
    REQUIRE(TexStreamInsertableKey<ns::si00 &>);
    REQUIRE(TexStreamInsertableKey<const ns::si00 &>);
    REQUIRE(TexStreamInsertableKey<ns::si00 &&>);

    REQUIRE(TexStreamInsertableKey<ns::si00a>);
    REQUIRE(TexStreamInsertableKey<ns::si00a &>);
    REQUIRE(TexStreamInsertableKey<const ns::si00a &>);
    REQUIRE(TexStreamInsertableKey<ns::si00a &&>);

    REQUIRE(!TexStreamInsertableKey<ns::si01>);
    REQUIRE(TexStreamInsertableKey<ns::si01 &>);
    REQUIRE(!TexStreamInsertableKey<const ns::si01 &>);
    REQUIRE(!TexStreamInsertableKey<ns::si01 &&>);

    REQUIRE(!TexStreamInsertableKey<ns::nsi00>);
    REQUIRE(!TexStreamInsertableKey<ns::nsi00 &>);
    REQUIRE(!TexStreamInsertableKey<const ns::nsi00 &>);
    REQUIRE(!TexStreamInsertableKey<ns::nsi00 &&>);

    REQUIRE(TexStreamInsertableKey<ns::nsi01>);
    REQUIRE(TexStreamInsertableKey<ns::nsi01 &>);
    REQUIRE(TexStreamInsertableKey<const ns::nsi01 &>);
    REQUIRE(TexStreamInsertableKey<ns::nsi01 &&>);

    REQUIRE(TexStreamInsertableKey<ext_si00>);
    REQUIRE(TexStreamInsertableKey<ext_si00 &>);
    REQUIRE(TexStreamInsertableKey<const ext_si00 &>);
    REQUIRE(TexStreamInsertableKey<ext_si00 &&>);

    REQUIRE(!TexStreamInsertableKey<ext_si01>);
    REQUIRE(TexStreamInsertableKey<ext_si01 &>);
    REQUIRE(TexStreamInsertableKey<const ext_si01 &>);
    REQUIRE(!TexStreamInsertableKey<ext_si01 &&>);

    REQUIRE(!TexStreamInsertableKey<ext_nsi00>);
    REQUIRE(!TexStreamInsertableKey<ext_nsi00 &>);
    REQUIRE(!TexStreamInsertableKey<const ext_nsi00 &>);
    REQUIRE(!TexStreamInsertableKey<ext_nsi00 &&>);
#endif
}
