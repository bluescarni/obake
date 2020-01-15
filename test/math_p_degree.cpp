// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>

#include <obake/config.hpp>
#include <obake/math/p_degree.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("p_degree_arith")
{
    REQUIRE(!is_with_p_degree_v<void>);

    REQUIRE(!is_with_p_degree_v<int>);
    REQUIRE(!is_with_p_degree_v<int &>);
    REQUIRE(!is_with_p_degree_v<const int &>);
    REQUIRE(!is_with_p_degree_v<int &&>);
    REQUIRE(!is_with_p_degree_v<const int>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!WithPDegree<void>);

    REQUIRE(!WithPDegree<int>);
    REQUIRE(!WithPDegree<int &>);
    REQUIRE(!WithPDegree<const int &>);
    REQUIRE(!WithPDegree<int &&>);
    REQUIRE(!WithPDegree<const int>);
#endif
}

struct no_p_degree_0 {
};

// OK ADL implementation.
struct p_degree_0 {
};

// Wrong ADL.
struct no_p_degree_1 {
};

int p_degree(const p_degree_0 &, const symbol_set &);

int p_degree(const no_p_degree_1 &, symbol_set &);

// External customisation point.
struct p_degree_1 {
};

// Wrong ADL, correct custom point.
// OK ADL implementation.
struct p_degree_2 {
};

int p_degree(const p_degree_2 &, symbol_set &);

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, p_degree_1> inline constexpr auto p_degree<T>
#else
inline constexpr auto p_degree<T, std::enable_if_t<is_same_cvr_v<T, p_degree_1>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, p_degree_2> inline constexpr auto p_degree<T>
#else
inline constexpr auto p_degree<T, std::enable_if_t<is_same_cvr_v<T, p_degree_2>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return true;
};

} // namespace obake::customisation

TEST_CASE("p_degree_custom")
{
    // Check type-traits/concepts.
    REQUIRE(!is_with_p_degree_v<no_p_degree_0>);
    REQUIRE(is_with_p_degree_v<p_degree_0>);
    REQUIRE(is_with_p_degree_v<p_degree_1>);
    REQUIRE(!is_with_p_degree_v<no_p_degree_1>);
    REQUIRE(is_with_p_degree_v<p_degree_2>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!WithPDegree<no_p_degree_0>);
    REQUIRE(WithPDegree<p_degree_0>);
    REQUIRE(WithPDegree<p_degree_1>);
    REQUIRE(!WithPDegree<no_p_degree_1>);
    REQUIRE(WithPDegree<p_degree_2>);
#endif
}
