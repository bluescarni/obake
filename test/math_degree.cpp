// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/math/degree.hpp>
#include <piranha/type_traits.hpp>

#include "catch.hpp"

using namespace piranha;

TEST_CASE("degree_arith")
{
    REQUIRE(!is_with_degree_v<void>);

    REQUIRE(!is_with_degree_v<int>);
    REQUIRE(!is_with_degree_v<int &>);
    REQUIRE(!is_with_degree_v<const int &>);
    REQUIRE(!is_with_degree_v<int &&>);
    REQUIRE(!is_with_degree_v<const int>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!WithDegree<void>);

    REQUIRE(!WithDegree<int>);
    REQUIRE(!WithDegree<int &>);
    REQUIRE(!WithDegree<const int &>);
    REQUIRE(!WithDegree<int &&>);
    REQUIRE(!WithDegree<const int>);
#endif
}

struct no_degree_0 {
};

// OK ADL implementation.
struct degree_0 {
};

int degree(const degree_0 &);

// External customisation point.
struct degree_1 {
};

namespace piranha::customisation
{

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, degree_1> inline constexpr auto degree<T>
#else
inline constexpr auto degree<T, std::enable_if_t<is_same_cvr_v<T, degree_1>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return true;
};

} // namespace piranha::customisation

TEST_CASE("degree_custom")
{
    // Check type-traits/concepts.
    REQUIRE(!is_with_degree_v<no_degree_0>);
    REQUIRE(is_with_degree_v<degree_0>);
    REQUIRE(is_with_degree_v<degree_1>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!WithDegree<no_degree_0>);
    REQUIRE(WithDegree<degree_0>);
    REQUIRE(WithDegree<degree_1>);
#endif
}
