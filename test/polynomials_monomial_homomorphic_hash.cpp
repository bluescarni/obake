// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/polynomials/monomial_homomorphic_hash.hpp>

#include "catch.hpp"

using namespace piranha;

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
struct monomial_hash_is_homomorphic<hh0> : std::true_type {
};

template <>
struct monomial_hash_is_homomorphic<hh2> : std::false_type {
};

// Wrong specialisation here, correct one in the
// customisation namespace.
template <>
struct monomial_hash_is_homomorphic<hh3> {
};

// Wrong specialisation.
template <>
struct monomial_hash_is_homomorphic<nhh0> {
};

// Wrong value type.
template <>
struct monomial_hash_is_homomorphic<nhh2> {
    static constexpr int value = 1;
};

namespace customisation
{

template <>
struct monomial_hash_is_homomorphic<hh1> : std::true_type {
};

template <>
struct monomial_hash_is_homomorphic<hh2> : std::true_type {
};

template <>
struct monomial_hash_is_homomorphic<hh3> : std::true_type {
};

// Wrong specialisation.
template <>
struct monomial_hash_is_homomorphic<nhh1> {
};

} // namespace customisation

} // namespace piranha

TEST_CASE("monomial_hash_homomorphism")
{
    REQUIRE(!monomial_has_homomorphic_hash_v<int>);
    REQUIRE(!monomial_has_homomorphic_hash_v<std::string>);

    REQUIRE(monomial_has_homomorphic_hash_v<hh0>);
    REQUIRE(monomial_has_homomorphic_hash_v<hh0 &>);
    REQUIRE(monomial_has_homomorphic_hash_v<hh0 &&>);
    REQUIRE(monomial_has_homomorphic_hash_v<const hh0 &>);

    REQUIRE(monomial_has_homomorphic_hash_v<hh1>);
    REQUIRE(monomial_has_homomorphic_hash_v<hh1 &>);
    REQUIRE(monomial_has_homomorphic_hash_v<hh1 &&>);
    REQUIRE(monomial_has_homomorphic_hash_v<const hh1 &>);

    REQUIRE(monomial_has_homomorphic_hash_v<hh2>);
    REQUIRE(monomial_has_homomorphic_hash_v<hh2 &>);
    REQUIRE(monomial_has_homomorphic_hash_v<hh2 &&>);
    REQUIRE(monomial_has_homomorphic_hash_v<const hh2 &&>);

    REQUIRE(!monomial_has_homomorphic_hash_v<nhh0>);
    REQUIRE(!monomial_has_homomorphic_hash_v<nhh0 &>);
    REQUIRE(!monomial_has_homomorphic_hash_v<nhh0 &&>);
    REQUIRE(!monomial_has_homomorphic_hash_v<const nhh0 &>);

    REQUIRE(!monomial_has_homomorphic_hash_v<nhh1>);
    REQUIRE(!monomial_has_homomorphic_hash_v<nhh1 &>);
    REQUIRE(!monomial_has_homomorphic_hash_v<nhh1 &&>);
    REQUIRE(!monomial_has_homomorphic_hash_v<const nhh1 &>);

    REQUIRE(monomial_has_homomorphic_hash_v<hh3>);
    REQUIRE(monomial_has_homomorphic_hash_v<hh3 &>);
    REQUIRE(monomial_has_homomorphic_hash_v<hh3 &&>);
    REQUIRE(monomial_has_homomorphic_hash_v<const hh3 &&>);

    REQUIRE(!monomial_has_homomorphic_hash_v<nhh2>);
    REQUIRE(!monomial_has_homomorphic_hash_v<nhh2 &>);
    REQUIRE(!monomial_has_homomorphic_hash_v<nhh2 &&>);
    REQUIRE(!monomial_has_homomorphic_hash_v<const nhh2 &>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!MonomialHasHomomorphicHash<int>);
    REQUIRE(!MonomialHasHomomorphicHash<std::string>);

    REQUIRE(MonomialHasHomomorphicHash<hh0>);
    REQUIRE(MonomialHasHomomorphicHash<hh0 &>);
    REQUIRE(MonomialHasHomomorphicHash<hh0 &&>);
    REQUIRE(MonomialHasHomomorphicHash<const hh0 &>);

    REQUIRE(MonomialHasHomomorphicHash<hh1>);
    REQUIRE(MonomialHasHomomorphicHash<hh1 &>);
    REQUIRE(MonomialHasHomomorphicHash<hh1 &&>);
    REQUIRE(MonomialHasHomomorphicHash<const hh1 &>);

    REQUIRE(MonomialHasHomomorphicHash<hh2>);
    REQUIRE(MonomialHasHomomorphicHash<hh2 &>);
    REQUIRE(MonomialHasHomomorphicHash<hh2 &&>);
    REQUIRE(MonomialHasHomomorphicHash<const hh2 &&>);

    REQUIRE(!MonomialHasHomomorphicHash<nhh0>);
    REQUIRE(!MonomialHasHomomorphicHash<nhh0 &>);
    REQUIRE(!MonomialHasHomomorphicHash<nhh0 &&>);
    REQUIRE(!MonomialHasHomomorphicHash<const nhh0 &>);

    REQUIRE(!MonomialHasHomomorphicHash<nhh1>);
    REQUIRE(!MonomialHasHomomorphicHash<nhh1 &>);
    REQUIRE(!MonomialHasHomomorphicHash<nhh1 &&>);
    REQUIRE(!MonomialHasHomomorphicHash<const nhh1 &>);

    REQUIRE(MonomialHasHomomorphicHash<hh3>);
    REQUIRE(MonomialHasHomomorphicHash<hh3 &>);
    REQUIRE(MonomialHasHomomorphicHash<hh3 &&>);
    REQUIRE(MonomialHasHomomorphicHash<const hh3 &&>);

    REQUIRE(!MonomialHasHomomorphicHash<nhh2>);
    REQUIRE(!MonomialHasHomomorphicHash<nhh2 &>);
    REQUIRE(!MonomialHasHomomorphicHash<nhh2 &&>);
    REQUIRE(!MonomialHasHomomorphicHash<const nhh2 &>);
#endif
}
