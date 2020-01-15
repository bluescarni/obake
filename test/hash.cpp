// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <obake/config.hpp>
#include <obake/hash.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

struct nohash_00 {
};

namespace ns
{

struct hash_00 {
};

std::size_t hash(const hash_00 &);

struct hash_01 {
};

std::size_t hash(hash_01 &);

struct hash_02 {
};

// Wrong return type.
void hash(const hash_02 &);

struct hash_03 {
};

// Wrong return type.
int hash(const hash_03 &);

} // namespace ns

struct hash_ext {
};

struct nohash_ext_00 {
};

struct nohash_ext_01 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, hash_ext> inline constexpr auto hash<T>
#else
inline constexpr auto hash<T, std::enable_if_t<is_same_cvr_v<T, hash_ext>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return std::size_t(0);
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, nohash_ext_00> inline constexpr auto hash<T>
#else
inline constexpr auto hash<T, std::enable_if_t<is_same_cvr_v<T, nohash_ext_00>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return 1;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, nohash_ext_01> inline constexpr auto hash<T>
#else
inline constexpr auto hash<T, std::enable_if_t<is_same_cvr_v<T, nohash_ext_01>>>
#endif
    = [](nohash_ext_01 &) constexpr noexcept
{
    return std::size_t(0);
};

} // namespace obake::customisation

TEST_CASE("hash_test")
{
    REQUIRE(!is_hashable_v<void>);

    REQUIRE(is_hashable_v<int>);
    REQUIRE(is_hashable_v<int &>);
    REQUIRE(is_hashable_v<const int &>);
    REQUIRE(is_hashable_v<int &&>);
    REQUIRE(hash(42) == std::hash<int>{}(42));

    REQUIRE(is_hashable_v<std::string>);
    REQUIRE(is_hashable_v<std::string &>);
    REQUIRE(is_hashable_v<const std::string &>);
    REQUIRE(is_hashable_v<std::string &&>);
    REQUIRE(hash(std::string{"hello world"}) == std::hash<std::string>{}(std::string{"hello world"}));

    REQUIRE(is_hashable_v<mppp::integer<1>>);
    REQUIRE(is_hashable_v<mppp::rational<1>>);
    REQUIRE(hash(mppp::integer<1>{123}) == mppp::hash(mppp::integer<1>{123}));
    REQUIRE(hash(mppp::rational<1>{123, -456}) == mppp::hash(mppp::rational<1>{123, -456}));

    REQUIRE(!is_hashable_v<nohash_00>);
    REQUIRE(!is_hashable_v<nohash_00 &>);
    REQUIRE(!is_hashable_v<const nohash_00 &>);
    REQUIRE(!is_hashable_v<nohash_00 &&>);

    REQUIRE(is_hashable_v<ns::hash_00>);
    REQUIRE(is_hashable_v<ns::hash_00 &>);
    REQUIRE(is_hashable_v<const ns::hash_00 &>);
    REQUIRE(is_hashable_v<ns::hash_00 &&>);

    REQUIRE(!is_hashable_v<ns::hash_01>);
    REQUIRE(is_hashable_v<ns::hash_01 &>);
    REQUIRE(!is_hashable_v<const ns::hash_01 &>);
    REQUIRE(!is_hashable_v<ns::hash_01 &&>);

    REQUIRE(!is_hashable_v<ns::hash_02>);
    REQUIRE(!is_hashable_v<ns::hash_02 &>);
    REQUIRE(!is_hashable_v<ns::hash_02 &>);
    REQUIRE(!is_hashable_v<ns::hash_02 &&>);

    REQUIRE(!is_hashable_v<ns::hash_03>);
    REQUIRE(!is_hashable_v<ns::hash_03 &>);
    REQUIRE(!is_hashable_v<ns::hash_03 &>);
    REQUIRE(!is_hashable_v<ns::hash_03 &&>);

    REQUIRE(is_hashable_v<hash_ext>);
    REQUIRE(is_hashable_v<hash_ext &>);
    REQUIRE(is_hashable_v<const hash_ext &>);
    REQUIRE(is_hashable_v<hash_ext &&>);
    REQUIRE(hash(hash_ext{}) == 0u);

    REQUIRE(!is_hashable_v<nohash_ext_00>);
    REQUIRE(!is_hashable_v<nohash_ext_00 &>);
    REQUIRE(!is_hashable_v<const nohash_ext_00 &>);
    REQUIRE(!is_hashable_v<nohash_ext_00 &&>);

    REQUIRE(!is_hashable_v<nohash_ext_01>);
    REQUIRE(is_hashable_v<nohash_ext_01 &>);
    REQUIRE(!is_hashable_v<const nohash_ext_01 &>);
    REQUIRE(!is_hashable_v<nohash_ext_01 &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Hashable<void>);

    REQUIRE(Hashable<int>);
    REQUIRE(Hashable<int &>);
    REQUIRE(Hashable<const int &>);
    REQUIRE(Hashable<int &&>);

    REQUIRE(Hashable<std::string>);
    REQUIRE(Hashable<std::string &>);
    REQUIRE(Hashable<const std::string &>);
    REQUIRE(Hashable<std::string &&>);

    REQUIRE(Hashable<mppp::integer<1>>);
    REQUIRE(Hashable<mppp::rational<1>>);

    REQUIRE(!Hashable<nohash_00>);
    REQUIRE(!Hashable<nohash_00 &>);
    REQUIRE(!Hashable<const nohash_00 &>);
    REQUIRE(!Hashable<nohash_00 &&>);

    REQUIRE(Hashable<ns::hash_00>);
    REQUIRE(Hashable<ns::hash_00 &>);
    REQUIRE(Hashable<const ns::hash_00 &>);
    REQUIRE(Hashable<ns::hash_00 &&>);

    REQUIRE(!Hashable<ns::hash_01>);
    REQUIRE(Hashable<ns::hash_01 &>);
    REQUIRE(!Hashable<const ns::hash_01 &>);
    REQUIRE(!Hashable<ns::hash_01 &&>);

    REQUIRE(!Hashable<ns::hash_02>);
    REQUIRE(!Hashable<ns::hash_02 &>);
    REQUIRE(!Hashable<ns::hash_02 &>);
    REQUIRE(!Hashable<ns::hash_02 &&>);

    REQUIRE(!Hashable<ns::hash_03>);
    REQUIRE(!Hashable<ns::hash_03 &>);
    REQUIRE(!Hashable<ns::hash_03 &>);
    REQUIRE(!Hashable<ns::hash_03 &&>);

    REQUIRE(Hashable<hash_ext>);
    REQUIRE(Hashable<hash_ext &>);
    REQUIRE(Hashable<const hash_ext &>);
    REQUIRE(Hashable<hash_ext &&>);

    REQUIRE(!Hashable<nohash_ext_00>);
    REQUIRE(!Hashable<nohash_ext_00 &>);
    REQUIRE(!Hashable<const nohash_ext_00 &>);
    REQUIRE(!Hashable<nohash_ext_00 &&>);

    REQUIRE(!Hashable<nohash_ext_01>);
    REQUIRE(Hashable<nohash_ext_01 &>);
    REQUIRE(!Hashable<const nohash_ext_01 &>);
    REQUIRE(!Hashable<nohash_ext_01 &&>);
#endif
}
