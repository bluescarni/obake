// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/cast.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <limits>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

// Non-castable.
struct noncast00 {
};

namespace ns
{

struct cast00 {
};

template <typename T>
T cast(cast00);

struct noncast01 {
};

// Wrong return type.
template <typename T>
void cast(noncast01);

struct cast01 {
};

// Valid only on some overloads.
template <typename T>
T cast(cast01 &);

} // namespace ns

// Customisation points.
struct extcast00 {
};

struct no_extcast00 {
};

struct intcast00 {
};

struct no_intcast00 {
};

namespace piranha::customisation
{

template <typename From>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<From, extcast00> inline constexpr auto cast<int, From>
#else
inline constexpr auto cast<int, From, std::enable_if_t<is_same_cvr_v<From, extcast00>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return 42;
};

template <typename From>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<From, no_extcast00> inline constexpr auto cast<int, From>
#else
inline constexpr auto cast<int, From, std::enable_if_t<is_same_cvr_v<From, no_extcast00>>>
#endif
    = [](auto &&) constexpr noexcept
{
    // Return mismatched type.
    return 42.;
};

namespace internal
{

template <typename From>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<From, intcast00> inline constexpr auto cast<double, From>
#else
inline constexpr auto cast<double, From, std::enable_if_t<is_same_cvr_v<From, intcast00>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return -42.;
};

template <typename From>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<From, no_intcast00> inline constexpr auto cast<double, From>
#else
inline constexpr auto cast<double, From, std::enable_if_t<is_same_cvr_v<From, no_intcast00>>>
#endif
    = [](auto &&) constexpr noexcept
{
    // Return mismatched type.
    return -42;
};

} // namespace internal

} // namespace piranha::customisation

TEST_CASE("cast_test")
{
    REQUIRE(cast<int>(5.6) == 5);
    REQUIRE(std::is_same_v<int, decltype(cast<int>(5.6))>);
    REQUIRE(cast<double>(-1) == -1.);
    REQUIRE(std::is_same_v<double, decltype(cast<double>(-1))>);
    REQUIRE(cast<unsigned>(-1) == std::numeric_limits<unsigned>::max());

    REQUIRE(!is_castable_v<void, void>);
    REQUIRE(!is_castable_v<int, void>);
    REQUIRE(!is_castable_v<void, int>);

    REQUIRE(is_castable_v<int, double>);
    REQUIRE(is_castable_v<int &, double>);
    REQUIRE(is_castable_v<const int &, double>);
    REQUIRE(!is_castable_v<int, const double>);
    REQUIRE(!is_castable_v<int &, volatile double>);
    REQUIRE(!is_castable_v<const int &, const volatile double>);

    REQUIRE(is_castable_v<double, int>);
    REQUIRE(is_castable_v<double &, int>);
    REQUIRE(is_castable_v<const double &, int>);
    REQUIRE(!is_castable_v<double, const int>);
    REQUIRE(!is_castable_v<double &, volatile int>);
    REQUIRE(!is_castable_v<const double &, const volatile int>);

    REQUIRE(!is_castable_v<int, double &>);
    REQUIRE(!is_castable_v<int &, const double>);
    REQUIRE(!is_castable_v<const int &, volatile double>);

    REQUIRE(!is_castable_v<int, noncast00>);
    REQUIRE(!is_castable_v<int &, noncast00>);
    REQUIRE(!is_castable_v<const int &, noncast00>);

    REQUIRE(is_castable_v<ns::cast00, noncast00>);
    REQUIRE(is_castable_v<ns::cast00 &, noncast00>);
    REQUIRE(is_castable_v<const ns::cast00 &&, noncast00>);

    REQUIRE(is_castable_v<ns::cast00, int>);
    REQUIRE(is_castable_v<ns::cast00 &, int>);
    REQUIRE(is_castable_v<const ns::cast00 &&, int>);

    REQUIRE(!is_castable_v<ns::cast00, void>);
    REQUIRE(!is_castable_v<ns::cast00 &, void>);
    REQUIRE(!is_castable_v<const ns::cast00 &&, void>);

    REQUIRE(!is_castable_v<ns::cast00, int &>);
    REQUIRE(!is_castable_v<ns::cast00 &, const int>);
    REQUIRE(!is_castable_v<const ns::cast00 &&, volatile int>);

    REQUIRE(!is_castable_v<ns::noncast01, int>);
    REQUIRE(!is_castable_v<ns::noncast01 &, int>);
    REQUIRE(!is_castable_v<const ns::noncast01 &&, int>);

    REQUIRE(!is_castable_v<ns::cast01, int>);
    REQUIRE(is_castable_v<ns::cast01 &, int>);
    REQUIRE(!is_castable_v<const ns::cast01 &, int>);

    REQUIRE(is_castable_v<extcast00, int>);
    REQUIRE(is_castable_v<extcast00 &, int>);
    REQUIRE(is_castable_v<const extcast00 &, int>);
    REQUIRE(!is_castable_v<const extcast00 &, double>);
    REQUIRE(cast<int>(extcast00{}) == 42);

    REQUIRE(is_castable_v<intcast00, double>);
    REQUIRE(is_castable_v<intcast00 &, double>);
    REQUIRE(is_castable_v<const intcast00 &, double>);
    REQUIRE(!is_castable_v<const intcast00 &, int>);
    REQUIRE(cast<double>(intcast00{}) == -42.);

    REQUIRE(!is_castable_v<no_extcast00, int>);
    REQUIRE(!is_castable_v<no_extcast00 &, int>);
    REQUIRE(!is_castable_v<const no_extcast00 &, int>);
    REQUIRE(!is_castable_v<const no_extcast00 &, double>);

    REQUIRE(!is_castable_v<no_intcast00, double>);
    REQUIRE(!is_castable_v<no_intcast00 &, double>);
    REQUIRE(!is_castable_v<const no_intcast00 &, double>);
    REQUIRE(!is_castable_v<const no_intcast00 &, int>);

#if defined(PIRANHA_HAVE_CONCEPTS)

    REQUIRE(!Castable<void, void>);
    REQUIRE(!Castable<int, void>);
    REQUIRE(!Castable<void, int>);

    REQUIRE(Castable<int, double>);
    REQUIRE(Castable<int &, double>);
    REQUIRE(Castable<const int &, double>);
    REQUIRE(!Castable<int, const double>);
    REQUIRE(!Castable<int &, volatile double>);
    REQUIRE(!Castable<const int &, const volatile double>);

    REQUIRE(Castable<double, int>);
    REQUIRE(Castable<double &, int>);
    REQUIRE(Castable<const double &, int>);
    REQUIRE(!Castable<double, const int>);
    REQUIRE(!Castable<double &, volatile int>);
    REQUIRE(!Castable<const double &, const volatile int>);

    REQUIRE(!Castable<int, double &>);
    REQUIRE(!Castable<int &, const double>);
    REQUIRE(!Castable<const int &, volatile double>);

    REQUIRE(!Castable<int, noncast00>);
    REQUIRE(!Castable<int &, noncast00>);
    REQUIRE(!Castable<const int &, noncast00>);

    REQUIRE(Castable<ns::cast00, noncast00>);
    REQUIRE(Castable<ns::cast00 &, noncast00>);
    REQUIRE(Castable<const ns::cast00 &&, noncast00>);

    REQUIRE(Castable<ns::cast00, int>);
    REQUIRE(Castable<ns::cast00 &, int>);
    REQUIRE(Castable<const ns::cast00 &&, int>);

    REQUIRE(!Castable<ns::cast00, void>);
    REQUIRE(!Castable<ns::cast00 &, void>);
    REQUIRE(!Castable<const ns::cast00 &&, void>);

    REQUIRE(!Castable<ns::cast00, int &>);
    REQUIRE(!Castable<ns::cast00 &, const int>);
    REQUIRE(!Castable<const ns::cast00 &&, volatile int>);

    REQUIRE(!Castable<ns::noncast01, int>);
    REQUIRE(!Castable<ns::noncast01 &, int>);
    REQUIRE(!Castable<const ns::noncast01 &&, int>);

    REQUIRE(!Castable<ns::cast01, int>);
    REQUIRE(Castable<ns::cast01 &, int>);
    REQUIRE(!Castable<const ns::cast01 &, int>);

    REQUIRE(Castable<extcast00, int>);
    REQUIRE(Castable<extcast00 &, int>);
    REQUIRE(Castable<const extcast00 &, int>);
    REQUIRE(!Castable<const extcast00 &, double>);

    REQUIRE(Castable<intcast00, double>);
    REQUIRE(Castable<intcast00 &, double>);
    REQUIRE(Castable<const intcast00 &, double>);
    REQUIRE(!Castable<const intcast00 &, int>);

    REQUIRE(!Castable<no_extcast00, int>);
    REQUIRE(!Castable<no_extcast00 &, int>);
    REQUIRE(!Castable<const no_extcast00 &, int>);
    REQUIRE(!Castable<const no_extcast00 &, double>);

    REQUIRE(!Castable<no_intcast00, double>);
    REQUIRE(!Castable<no_intcast00 &, double>);
    REQUIRE(!Castable<const no_intcast00 &, double>);
    REQUIRE(!Castable<const no_intcast00 &, int>);

#endif
}
