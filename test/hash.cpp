// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
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

#include <piranha/config.hpp>
#include <piranha/hash.hpp>
#include <piranha/type_traits.hpp>

#include "catch.hpp"

using namespace piranha;

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

namespace piranha::customisation
{

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, hash_ext> inline constexpr auto hash<T>
#else
inline constexpr auto hash<T, std::enable_if_t<is_same_cvr_v<T, hash_ext>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return std::size_t(0);
};

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, nohash_ext_00> inline constexpr auto hash<T>
#else
inline constexpr auto hash<T, std::enable_if_t<is_same_cvr_v<T, nohash_ext_00>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return 1;
};

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, nohash_ext_01> inline constexpr auto hash<T>
#else
inline constexpr auto hash<T, std::enable_if_t<is_same_cvr_v<T, nohash_ext_01>>>
#endif
    = [](nohash_ext_01 &) constexpr noexcept
{
    return std::size_t(0);
};

} // namespace piranha::customisation

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

#if defined(PIRANHA_HAVE_CONCEPTS)
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

struct hh0 {
};

struct hh1 {
};

struct hh2 {
};

struct hh3 {
};

struct nhh0 {
};

struct nhh1 {
};

struct nhh2 {
};

namespace piranha
{

template <>
struct hash_is_homomorphic<hh0> : std::true_type {
};

template <>
struct hash_is_homomorphic<hh2> : std::false_type {
};

// Wrong specialisation here, correct one in the
// customisation namespace.
template <>
struct hash_is_homomorphic<hh3> {
};

// Wrong specialisation.
template <>
struct hash_is_homomorphic<nhh0> {
};

// Wrong value type.
template <>
struct hash_is_homomorphic<nhh2> {
    static constexpr int value = 1;
};

namespace customisation
{

template <>
struct hash_is_homomorphic<hh1> : std::true_type {
};

template <>
struct hash_is_homomorphic<hh2> : std::true_type {
};

template <>
struct hash_is_homomorphic<hh3> : std::true_type {
};

// Wrong specialisation.
template <>
struct hash_is_homomorphic<nhh1> {
};

} // namespace customisation

} // namespace piranha

TEST_CASE("hash_homomorphism")
{
    REQUIRE(!has_homomorphic_hash_v<int>);
    REQUIRE(!has_homomorphic_hash_v<std::string>);

    REQUIRE(has_homomorphic_hash_v<hh0>);
    REQUIRE(has_homomorphic_hash_v<hh0 &>);
    REQUIRE(has_homomorphic_hash_v<hh0 &&>);
    REQUIRE(has_homomorphic_hash_v<const hh0 &>);

    REQUIRE(has_homomorphic_hash_v<hh1>);
    REQUIRE(has_homomorphic_hash_v<hh1 &>);
    REQUIRE(has_homomorphic_hash_v<hh1 &&>);
    REQUIRE(has_homomorphic_hash_v<const hh1 &>);

    REQUIRE(has_homomorphic_hash_v<hh2>);
    REQUIRE(has_homomorphic_hash_v<hh2 &>);
    REQUIRE(has_homomorphic_hash_v<hh2 &&>);
    REQUIRE(has_homomorphic_hash_v<const hh2 &&>);

    REQUIRE(!has_homomorphic_hash_v<nhh0>);
    REQUIRE(!has_homomorphic_hash_v<nhh0 &>);
    REQUIRE(!has_homomorphic_hash_v<nhh0 &&>);
    REQUIRE(!has_homomorphic_hash_v<const nhh0 &>);

    REQUIRE(!has_homomorphic_hash_v<nhh1>);
    REQUIRE(!has_homomorphic_hash_v<nhh1 &>);
    REQUIRE(!has_homomorphic_hash_v<nhh1 &&>);
    REQUIRE(!has_homomorphic_hash_v<const nhh1 &>);

    REQUIRE(has_homomorphic_hash_v<hh3>);
    REQUIRE(has_homomorphic_hash_v<hh3 &>);
    REQUIRE(has_homomorphic_hash_v<hh3 &&>);
    REQUIRE(has_homomorphic_hash_v<const hh3 &&>);

    REQUIRE(!has_homomorphic_hash_v<nhh2>);
    REQUIRE(!has_homomorphic_hash_v<nhh2 &>);
    REQUIRE(!has_homomorphic_hash_v<nhh2 &&>);
    REQUIRE(!has_homomorphic_hash_v<const nhh2 &>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!HasHomomorphicHash<int>);
    REQUIRE(!HasHomomorphicHash<std::string>);

    REQUIRE(HasHomomorphicHash<hh0>);
    REQUIRE(HasHomomorphicHash<hh0 &>);
    REQUIRE(HasHomomorphicHash<hh0 &&>);
    REQUIRE(HasHomomorphicHash<const hh0 &>);

    REQUIRE(HasHomomorphicHash<hh1>);
    REQUIRE(HasHomomorphicHash<hh1 &>);
    REQUIRE(HasHomomorphicHash<hh1 &&>);
    REQUIRE(HasHomomorphicHash<const hh1 &>);

    REQUIRE(HasHomomorphicHash<hh2>);
    REQUIRE(HasHomomorphicHash<hh2 &>);
    REQUIRE(HasHomomorphicHash<hh2 &&>);
    REQUIRE(HasHomomorphicHash<const hh2 &&>);

    REQUIRE(!HasHomomorphicHash<nhh0>);
    REQUIRE(!HasHomomorphicHash<nhh0 &>);
    REQUIRE(!HasHomomorphicHash<nhh0 &&>);
    REQUIRE(!HasHomomorphicHash<const nhh0 &>);

    REQUIRE(!HasHomomorphicHash<nhh1>);
    REQUIRE(!HasHomomorphicHash<nhh1 &>);
    REQUIRE(!HasHomomorphicHash<nhh1 &&>);
    REQUIRE(!HasHomomorphicHash<const nhh1 &>);

    REQUIRE(HasHomomorphicHash<hh3>);
    REQUIRE(HasHomomorphicHash<hh3 &>);
    REQUIRE(HasHomomorphicHash<hh3 &&>);
    REQUIRE(HasHomomorphicHash<const hh3 &&>);

    REQUIRE(!HasHomomorphicHash<nhh2>);
    REQUIRE(!HasHomomorphicHash<nhh2 &>);
    REQUIRE(!HasHomomorphicHash<nhh2 &&>);
    REQUIRE(!HasHomomorphicHash<const nhh2 &>);
#endif
}
