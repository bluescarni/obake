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
#include <obake/polynomials/monomial_homomorphic_hash.hpp>

#include "catch.hpp"

using namespace obake;

struct hh0 {
};

struct hh1 {
};

struct hh2 {
};

struct hh3 {
};

struct hh4 {
};

struct nhh0 {
};

struct nhh1 {
};

struct nhh2 {
};

namespace obake
{

template <>
inline constexpr bool monomial_hash_is_homomorphic<hh0> = true;

template <>
inline constexpr bool monomial_hash_is_homomorphic<hh2> = false;

// Wrong specialisation here, correct one in the
// customisation namespace.
template <>
inline constexpr int monomial_hash_is_homomorphic<hh3> = 3;

// Not constexpr here, constexpr in customisation.
template <>
inline bool monomial_hash_is_homomorphic<hh4> = false;

// Wrong specialisation.
template <>
inline constexpr int monomial_hash_is_homomorphic<nhh0> = 4;

namespace customisation
{

template <>
inline constexpr bool monomial_hash_is_homomorphic<hh1> = true;

template <>
inline constexpr bool monomial_hash_is_homomorphic<hh2> = true;

template <>
inline constexpr bool monomial_hash_is_homomorphic<hh3> = true;

template <>
inline constexpr bool monomial_hash_is_homomorphic<hh4> = true;

// Wrong specialisation.
template <>
inline constexpr float monomial_hash_is_homomorphic<nhh1> = 1.f;

// Not constexpr.
template <>
inline bool monomial_hash_is_homomorphic<nhh2> = true;

} // namespace customisation

} // namespace obake

TEST_CASE("monomial_hash_homomorphism")
{
    REQUIRE(!is_homomorphically_hashable_monomial_v<int>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<std::string>);

    REQUIRE(is_homomorphically_hashable_monomial_v<hh0>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<hh0 &>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<hh0 &&>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<const hh0 &>);

    REQUIRE(is_homomorphically_hashable_monomial_v<hh1>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<hh1 &>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<hh1 &&>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<const hh1 &>);

    REQUIRE(is_homomorphically_hashable_monomial_v<hh2>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<hh2 &>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<hh2 &&>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<const hh2 &&>);

    REQUIRE(!is_homomorphically_hashable_monomial_v<nhh0>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<nhh0 &>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<nhh0 &&>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<const nhh0 &>);

    REQUIRE(!is_homomorphically_hashable_monomial_v<nhh1>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<nhh1 &>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<nhh1 &&>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<const nhh1 &>);

    REQUIRE(is_homomorphically_hashable_monomial_v<hh3>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<hh3 &>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<hh3 &&>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<const hh3 &&>);

    REQUIRE(is_homomorphically_hashable_monomial_v<hh4>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<hh4 &>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<hh4 &&>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<const hh4 &&>);

    REQUIRE(!is_homomorphically_hashable_monomial_v<nhh2>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<nhh2 &>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<nhh2 &&>);
    REQUIRE(!is_homomorphically_hashable_monomial_v<const nhh2 &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!HomomorphicallyHashableMonomial<int>);
    REQUIRE(!HomomorphicallyHashableMonomial<std::string>);

    REQUIRE(HomomorphicallyHashableMonomial<hh0>);
    REQUIRE(!HomomorphicallyHashableMonomial<hh0 &>);
    REQUIRE(!HomomorphicallyHashableMonomial<hh0 &&>);
    REQUIRE(!HomomorphicallyHashableMonomial<const hh0 &>);

    REQUIRE(HomomorphicallyHashableMonomial<hh1>);
    REQUIRE(!HomomorphicallyHashableMonomial<hh1 &>);
    REQUIRE(!HomomorphicallyHashableMonomial<hh1 &&>);
    REQUIRE(!HomomorphicallyHashableMonomial<const hh1 &>);

    REQUIRE(HomomorphicallyHashableMonomial<hh2>);
    REQUIRE(!HomomorphicallyHashableMonomial<hh2 &>);
    REQUIRE(!HomomorphicallyHashableMonomial<hh2 &&>);
    REQUIRE(!HomomorphicallyHashableMonomial<const hh2 &&>);

    REQUIRE(!HomomorphicallyHashableMonomial<nhh0>);
    REQUIRE(!HomomorphicallyHashableMonomial<nhh0 &>);
    REQUIRE(!HomomorphicallyHashableMonomial<nhh0 &&>);
    REQUIRE(!HomomorphicallyHashableMonomial<const nhh0 &>);

    REQUIRE(!HomomorphicallyHashableMonomial<nhh1>);
    REQUIRE(!HomomorphicallyHashableMonomial<nhh1 &>);
    REQUIRE(!HomomorphicallyHashableMonomial<nhh1 &&>);
    REQUIRE(!HomomorphicallyHashableMonomial<const nhh1 &>);

    REQUIRE(HomomorphicallyHashableMonomial<hh3>);
    REQUIRE(!HomomorphicallyHashableMonomial<hh3 &>);
    REQUIRE(!HomomorphicallyHashableMonomial<hh3 &&>);
    REQUIRE(!HomomorphicallyHashableMonomial<const hh3 &&>);

    REQUIRE(HomomorphicallyHashableMonomial<hh4>);
    REQUIRE(!HomomorphicallyHashableMonomial<hh4 &>);
    REQUIRE(!HomomorphicallyHashableMonomial<hh4 &&>);
    REQUIRE(!HomomorphicallyHashableMonomial<const hh4 &&>);

    REQUIRE(!HomomorphicallyHashableMonomial<nhh2>);
    REQUIRE(!HomomorphicallyHashableMonomial<nhh2 &>);
    REQUIRE(!HomomorphicallyHashableMonomial<nhh2 &&>);
    REQUIRE(!HomomorphicallyHashableMonomial<const nhh2 &>);
#endif
}
