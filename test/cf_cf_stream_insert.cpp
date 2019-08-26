// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>

#include <piranha/cf/cf_stream_insert.hpp>
#include <piranha/config.hpp>
#include <piranha/type_traits.hpp>

#include "catch.hpp"

using namespace piranha;

// ADL-based implementations.
namespace ns
{

struct si00 {
};

void cf_stream_insert(std::ostream &os, const si00 &);

struct si01 {
};

// Disable certain overloads.
void cf_stream_insert(std::ostream &os, si01 &);

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

namespace piranha::customisation
{

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, ext_si00> inline constexpr auto cf_stream_insert<T>
#else
inline constexpr auto cf_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ext_si00>>>
#endif
    = [](std::ostream &, auto &&) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, ext_si01> inline constexpr auto cf_stream_insert<T>
#else
inline constexpr auto cf_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ext_si01>>>
#endif
    = [](std::ostream &, auto &) constexpr noexcept
{
    return true;
};

namespace internal
{

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, int_si00> inline constexpr auto cf_stream_insert<T>
#else
inline constexpr auto cf_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, int_si00>>>
#endif
    = [](std::ostream &, auto &&) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, int_si01> inline constexpr auto cf_stream_insert<T>
#else
inline constexpr auto cf_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, int_si01>>>
#endif
    = [](std::ostream &, auto &) constexpr noexcept
{
    return true;
};

} // namespace internal

} // namespace piranha::customisation

TEST_CASE("cf_stream_insert_test")
{
    REQUIRE(!is_stream_insertable_cf_v<void>);

    REQUIRE(is_stream_insertable_cf_v<int>);
    REQUIRE(is_stream_insertable_cf_v<int &>);
    REQUIRE(is_stream_insertable_cf_v<const int &>);
    REQUIRE(is_stream_insertable_cf_v<std::string>);
    REQUIRE(is_stream_insertable_cf_v<std::string &>);
    REQUIRE(is_stream_insertable_cf_v<const std::string &>);

    std::ostringstream oss1, oss2;
    cf_stream_insert(oss1, 42);
    oss2 << 42;
    REQUIRE(oss1.str() == oss2.str());

    REQUIRE(is_stream_insertable_cf_v<ns::si00>);
    REQUIRE(is_stream_insertable_cf_v<ns::si00 &>);
    REQUIRE(is_stream_insertable_cf_v<const ns::si00 &>);
    REQUIRE(is_stream_insertable_cf_v<ns::si00 &&>);

    REQUIRE(!is_stream_insertable_cf_v<ns::si01>);
    REQUIRE(is_stream_insertable_cf_v<ns::si01 &>);
    REQUIRE(!is_stream_insertable_cf_v<const ns::si01 &>);
    REQUIRE(!is_stream_insertable_cf_v<ns::si01 &&>);

    REQUIRE(!is_stream_insertable_cf_v<ns::nsi00>);
    REQUIRE(!is_stream_insertable_cf_v<ns::nsi00 &>);
    REQUIRE(!is_stream_insertable_cf_v<const ns::nsi00 &>);
    REQUIRE(!is_stream_insertable_cf_v<ns::nsi00 &&>);

    REQUIRE(is_stream_insertable_cf_v<ext_si00>);
    REQUIRE(is_stream_insertable_cf_v<ext_si00 &>);
    REQUIRE(is_stream_insertable_cf_v<const ext_si00 &>);
    REQUIRE(is_stream_insertable_cf_v<ext_si00 &&>);

    REQUIRE(!is_stream_insertable_cf_v<ext_si01>);
    REQUIRE(is_stream_insertable_cf_v<ext_si01 &>);
    REQUIRE(is_stream_insertable_cf_v<const ext_si01 &>);
    REQUIRE(!is_stream_insertable_cf_v<ext_si01 &&>);

    REQUIRE(is_stream_insertable_cf_v<int_si00>);
    REQUIRE(is_stream_insertable_cf_v<int_si00 &>);
    REQUIRE(is_stream_insertable_cf_v<const int_si00 &>);
    REQUIRE(is_stream_insertable_cf_v<int_si00 &&>);

    REQUIRE(!is_stream_insertable_cf_v<int_si01>);
    REQUIRE(is_stream_insertable_cf_v<int_si01 &>);
    REQUIRE(is_stream_insertable_cf_v<const int_si01 &>);
    REQUIRE(!is_stream_insertable_cf_v<int_si01 &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!StreamInsertableCf<void>);

    REQUIRE(StreamInsertableCf<int>);
    REQUIRE(StreamInsertableCf<int &>);
    REQUIRE(StreamInsertableCf<const int &>);
    REQUIRE(StreamInsertableCf<std::string>);
    REQUIRE(StreamInsertableCf<std::string &>);
    REQUIRE(StreamInsertableCf<const std::string &>);

    REQUIRE(StreamInsertableCf<ns::si00>);
    REQUIRE(StreamInsertableCf<ns::si00 &>);
    REQUIRE(StreamInsertableCf<const ns::si00 &>);
    REQUIRE(StreamInsertableCf<ns::si00 &&>);

    REQUIRE(!StreamInsertableCf<ns::si01>);
    REQUIRE(StreamInsertableCf<ns::si01 &>);
    REQUIRE(!StreamInsertableCf<const ns::si01 &>);
    REQUIRE(!StreamInsertableCf<ns::si01 &&>);

    REQUIRE(!StreamInsertableCf<ns::nsi00>);
    REQUIRE(!StreamInsertableCf<ns::nsi00 &>);
    REQUIRE(!StreamInsertableCf<const ns::nsi00 &>);
    REQUIRE(!StreamInsertableCf<ns::nsi00 &&>);

    REQUIRE(StreamInsertableCf<ext_si00>);
    REQUIRE(StreamInsertableCf<ext_si00 &>);
    REQUIRE(StreamInsertableCf<const ext_si00 &>);
    REQUIRE(StreamInsertableCf<ext_si00 &&>);

    REQUIRE(!StreamInsertableCf<ext_si01>);
    REQUIRE(StreamInsertableCf<ext_si01 &>);
    REQUIRE(StreamInsertableCf<const ext_si01 &>);
    REQUIRE(!StreamInsertableCf<ext_si01 &&>);

    REQUIRE(StreamInsertableCf<int_si00>);
    REQUIRE(StreamInsertableCf<int_si00 &>);
    REQUIRE(StreamInsertableCf<const int_si00 &>);
    REQUIRE(StreamInsertableCf<int_si00 &&>);

    REQUIRE(!StreamInsertableCf<int_si01>);
    REQUIRE(StreamInsertableCf<int_si01 &>);
    REQUIRE(StreamInsertableCf<const int_si01 &>);
    REQUIRE(!StreamInsertableCf<int_si01 &&>);
#endif
}
