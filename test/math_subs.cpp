// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>

#include <obake/config.hpp>
#include <obake/math/subs.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("subs_arith")
{
    REQUIRE(!is_substitutable_v<void, void>);
    REQUIRE(!is_substitutable_v<void, int>);
    REQUIRE(!is_substitutable_v<int, void>);
    REQUIRE(!is_substitutable_v<int &, void>);
    REQUIRE(!is_substitutable_v<const int &, void>);
    REQUIRE(!is_substitutable_v<const int, void>);

    REQUIRE(is_substitutable_v<int, int>);
    REQUIRE(is_substitutable_v<int &, int>);
    REQUIRE(is_substitutable_v<const int &, int>);
    REQUIRE(is_substitutable_v<const int, int>);
    REQUIRE(!is_substitutable_v<int, int &>);
    REQUIRE(!is_substitutable_v<int, const int>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Substitutable<void, void>);
    REQUIRE(!Substitutable<void, int>);
    REQUIRE(!Substitutable<int, void>);
    REQUIRE(!Substitutable<int &, void>);
    REQUIRE(!Substitutable<const int &, void>);
    REQUIRE(!Substitutable<const int, void>);

    REQUIRE(Substitutable<int, int>);
    REQUIRE(Substitutable<int &, int>);
    REQUIRE(Substitutable<const int &, int>);
    REQUIRE(Substitutable<const int, int>);
    REQUIRE(!Substitutable<int, int &>);
    REQUIRE(!Substitutable<int, const int>);
#endif
}

struct subs_base {
};

// OK ADL implementation.
struct subs_0 {
};

int subs(const subs_0 &, const symbol_map<int> &);

// External customisation point.
struct subs_1 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, subs_1> inline constexpr auto subs<T, double>
#else
inline constexpr auto subs<T, double, std::enable_if_t<is_same_cvr_v<T, subs_1>>>
#endif
    = [](auto &&, const symbol_map<double> &) constexpr noexcept
{
    return true;
};

} // namespace obake::customisation

TEST_CASE("subs_custom")
{
    // Check type-traits/concepts.
    REQUIRE(is_substitutable_v<subs_base, int>);
    REQUIRE(!is_substitutable_v<subs_base, int &>);
    REQUIRE(is_substitutable_v<subs_0, int>);
    REQUIRE(!is_substitutable_v<subs_0, int &>);
    REQUIRE(!is_substitutable_v<subs_0, const int &>);
    REQUIRE(!is_substitutable_v<subs_0, const int>);
    REQUIRE(is_substitutable_v<subs_0, double>);
    REQUIRE(is_substitutable_v<subs_1, double>);
    REQUIRE(!is_substitutable_v<subs_1, double &>);
    REQUIRE(!is_substitutable_v<subs_1, const double &>);
    REQUIRE(!is_substitutable_v<subs_1, const double>);
    REQUIRE(is_substitutable_v<subs_1, int>);

    REQUIRE(std::is_same_v<int, decltype(obake::subs(subs_0{}, symbol_map<int>{}))>);
    REQUIRE(std::is_same_v<subs_0, decltype(obake::subs(subs_0{}, symbol_map<double>{}))>);
    REQUIRE(std::is_same_v<subs_1, decltype(obake::subs(subs_1{}, symbol_map<int>{}))>);
    REQUIRE(std::is_same_v<bool, decltype(obake::subs(subs_1{}, symbol_map<double>{}))>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(Substitutable<subs_base, int>);
    REQUIRE(!Substitutable<subs_base, int &>);
    REQUIRE(Substitutable<subs_0, int>);
    REQUIRE(Substitutable<subs_0, double>);
    REQUIRE(Substitutable<subs_1, double>);
    REQUIRE(Substitutable<subs_1, int>);
#endif
}
