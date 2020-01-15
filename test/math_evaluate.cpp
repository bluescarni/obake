// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>

#include <obake/config.hpp>
#include <obake/math/evaluate.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("evaluate_arith")
{
    REQUIRE(!is_evaluable_v<void, void>);
    REQUIRE(!is_evaluable_v<void, int>);
    REQUIRE(!is_evaluable_v<int, void>);
    REQUIRE(!is_evaluable_v<int &, void>);
    REQUIRE(!is_evaluable_v<const int &, void>);
    REQUIRE(!is_evaluable_v<const int, void>);

    REQUIRE(is_evaluable_v<int, int>);
    REQUIRE(is_evaluable_v<int &, int>);
    REQUIRE(is_evaluable_v<const int &, int>);
    REQUIRE(is_evaluable_v<const int, int>);
    REQUIRE(!is_evaluable_v<int, int &>);
    REQUIRE(!is_evaluable_v<int, const int>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Evaluable<void, void>);
    REQUIRE(!Evaluable<void, int>);
    REQUIRE(!Evaluable<int, void>);
    REQUIRE(!Evaluable<int &, void>);
    REQUIRE(!Evaluable<const int &, void>);
    REQUIRE(!Evaluable<const int, void>);

    REQUIRE(Evaluable<int, int>);
    REQUIRE(Evaluable<int &, int>);
    REQUIRE(Evaluable<const int &, int>);
    REQUIRE(Evaluable<const int, int>);
    REQUIRE(!Evaluable<int, int &>);
    REQUIRE(!Evaluable<int, const int>);
#endif
}

struct evaluate_base {
};

// OK ADL implementation.
struct evaluate_0 {
};

int evaluate(const evaluate_0 &, const symbol_map<int> &);

// External customisation point.
struct evaluate_1 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, evaluate_1> inline constexpr auto evaluate<T, double>
#else
inline constexpr auto evaluate<T, double, std::enable_if_t<is_same_cvr_v<T, evaluate_1>>>
#endif
    = [](auto &&, const symbol_map<double> &) constexpr noexcept
{
    return true;
};

} // namespace obake::customisation

TEST_CASE("evaluate_custom")
{
    // Check type-traits/concepts.
    REQUIRE(is_evaluable_v<evaluate_base, int>);
    REQUIRE(!is_evaluable_v<evaluate_base, int &>);
    REQUIRE(is_evaluable_v<evaluate_0, int>);
    REQUIRE(!is_evaluable_v<evaluate_0, int &>);
    REQUIRE(!is_evaluable_v<evaluate_0, const int &>);
    REQUIRE(!is_evaluable_v<evaluate_0, const int>);
    REQUIRE(is_evaluable_v<evaluate_0, double>);
    REQUIRE(is_evaluable_v<evaluate_1, double>);
    REQUIRE(!is_evaluable_v<evaluate_1, double &>);
    REQUIRE(!is_evaluable_v<evaluate_1, const double &>);
    REQUIRE(!is_evaluable_v<evaluate_1, const double>);
    REQUIRE(is_evaluable_v<evaluate_1, int>);

    REQUIRE(std::is_same_v<int, decltype(obake::evaluate(evaluate_0{}, symbol_map<int>{}))>);
    REQUIRE(std::is_same_v<evaluate_0, decltype(obake::evaluate(evaluate_0{}, symbol_map<double>{}))>);
    REQUIRE(std::is_same_v<evaluate_1, decltype(obake::evaluate(evaluate_1{}, symbol_map<int>{}))>);
    REQUIRE(std::is_same_v<bool, decltype(obake::evaluate(evaluate_1{}, symbol_map<double>{}))>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(Evaluable<evaluate_base, int>);
    REQUIRE(!Evaluable<evaluate_base, int &>);
    REQUIRE(Evaluable<evaluate_0, int>);
    REQUIRE(Evaluable<evaluate_0, double>);
    REQUIRE(Evaluable<evaluate_1, double>);
    REQUIRE(Evaluable<evaluate_1, int>);
#endif
}
