// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

// #include <limits>
// #include <type_traits>

#include <piranha/config.hpp>
#include <piranha/math/evaluate.hpp>
// #include <piranha/type_traits.hpp>

#include "catch.hpp"

using namespace piranha;

TEST_CASE("evaluate_arith")
{
    REQUIRE(!piranha::is_evaluable_v<void, void>);
    REQUIRE(!piranha::is_evaluable_v<void, int>);
    REQUIRE(!piranha::is_evaluable_v<int, void>);
    REQUIRE(!piranha::is_evaluable_v<int &, void>);
    REQUIRE(!piranha::is_evaluable_v<const int &, void>);
    REQUIRE(!piranha::is_evaluable_v<const int, void>);
}

#if 0

TEST_CASE("is_zero_mp++_int")
{
    using int_t = mppp::integer<1>;

    REQUIRE(piranha::is_zero_testable_v<int_t>);
    REQUIRE(piranha::is_zero_testable_v<int_t &>);
    REQUIRE(piranha::is_zero_testable_v<const int_t &>);
    REQUIRE(piranha::is_zero_testable_v<int_t &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::ZeroTestable<int_t>);
    REQUIRE(piranha::ZeroTestable<int_t &>);
    REQUIRE(piranha::ZeroTestable<const int_t &>);
    REQUIRE(piranha::ZeroTestable<int_t &&>);
#endif

    REQUIRE(piranha::is_zero(int_t{}));
    REQUIRE(!piranha::is_zero(int_t{-1}));
}

TEST_CASE("is_zero_mp++_rat")
{
    using rat_t = mppp::rational<1>;

    REQUIRE(piranha::is_zero_testable_v<rat_t>);
    REQUIRE(piranha::is_zero_testable_v<rat_t &>);
    REQUIRE(piranha::is_zero_testable_v<const rat_t &>);
    REQUIRE(piranha::is_zero_testable_v<rat_t &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::ZeroTestable<rat_t>);
    REQUIRE(piranha::ZeroTestable<rat_t &>);
    REQUIRE(piranha::ZeroTestable<const rat_t &>);
    REQUIRE(piranha::ZeroTestable<rat_t &&>);
#endif

    REQUIRE(piranha::is_zero(rat_t{}));
    REQUIRE(!piranha::is_zero(rat_t{-1, 45}));
}

#if defined(MPPP_WITH_MPFR)

TEST_CASE("is_zero_mp++_real")
{
    REQUIRE(piranha::is_zero_testable_v<mppp::real>);
    REQUIRE(piranha::is_zero_testable_v<mppp::real &>);
    REQUIRE(piranha::is_zero_testable_v<const mppp::real &>);
    REQUIRE(piranha::is_zero_testable_v<mppp::real &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::ZeroTestable<mppp::real>);
    REQUIRE(piranha::ZeroTestable<mppp::real &>);
    REQUIRE(piranha::ZeroTestable<const mppp::real &>);
    REQUIRE(piranha::ZeroTestable<mppp::real &&>);
#endif

    REQUIRE(piranha::is_zero(mppp::real{}));
    REQUIRE(!piranha::is_zero(mppp::real{42}));
    REQUIRE(!piranha::is_zero(mppp::real{"inf", 100}));
    REQUIRE(!piranha::is_zero(mppp::real{"nan", 100}));
}

#endif

#if defined(MPPP_WITH_QUADMATH)

TEST_CASE("is_zero_mp++_real128")
{
    REQUIRE(piranha::is_zero_testable_v<mppp::real128>);
    REQUIRE(piranha::is_zero_testable_v<mppp::real128 &>);
    REQUIRE(piranha::is_zero_testable_v<const mppp::real128 &>);
    REQUIRE(piranha::is_zero_testable_v<mppp::real128 &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::ZeroTestable<mppp::real128>);
    REQUIRE(piranha::ZeroTestable<mppp::real128 &>);
    REQUIRE(piranha::ZeroTestable<const mppp::real128 &>);
    REQUIRE(piranha::ZeroTestable<mppp::real128 &&>);
#endif

    REQUIRE(piranha::is_zero(mppp::real128{}));
    REQUIRE(!piranha::is_zero(mppp::real128{42}));
    REQUIRE(!piranha::is_zero(mppp::real128{"inf"}));
    REQUIRE(!piranha::is_zero(mppp::real128{"nan"}));

    // Verify constexpr.
    [[maybe_unused]] foo_b<piranha::is_zero(mppp::real128{})> tmp;
}

#endif

struct no_is_zero_0 {
};

// Wrong return type in the ADL implementation.
struct no_is_zero_1 {
};

void is_zero(const no_is_zero_1 &);

// OK ADL implementation.
struct is_zero_0 {
};

int is_zero(const is_zero_0 &);

// External customisation point.
struct is_zero_1 {
};

namespace piranha::customisation
{

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, is_zero_1> inline constexpr auto is_zero<T>
#else
inline constexpr auto is_zero<T, std::enable_if_t<is_same_cvr_v<T, is_zero_1>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return true;
};

} // namespace piranha::customisation

TEST_CASE("is_zero_custom")
{
    // Check type-traits/concepts.
    REQUIRE(!piranha::is_zero_testable_v<no_is_zero_0>);
    REQUIRE(!piranha::is_zero_testable_v<no_is_zero_1>);
    REQUIRE(piranha::is_zero_testable_v<is_zero_0>);
    REQUIRE(piranha::is_zero_testable_v<is_zero_1>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!piranha::ZeroTestable<no_is_zero_0>);
    REQUIRE(!piranha::ZeroTestable<no_is_zero_1>);
    REQUIRE(piranha::ZeroTestable<is_zero_0>);
    REQUIRE(piranha::ZeroTestable<is_zero_1>);
#endif
}

#endif