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
#include <obake/key/key_evaluate.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

// ADL-based implementations.
namespace ns
{

struct zt00 {
};

bool key_evaluate(const zt00 &, const symbol_idx_map<int> &, const symbol_set &);

struct zt01 {
};

// Disable certain overloads.
bool key_evaluate(zt01 &, const symbol_idx_map<int> &, const symbol_set &);

struct nzt00 {
};

// Wrong signature type.
std::string key_evaluate(nzt00 &, const symbol_set &);

} // namespace ns

struct ext_zt00 {
};

struct ext_zt01 {
};

struct ext_nzt00 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_zt00> inline constexpr auto key_evaluate<T, int>
#else
inline constexpr auto key_evaluate<T, int, std::enable_if_t<is_same_cvr_v<T, ext_zt00>>>
#endif
    = [](auto &&, const symbol_idx_map<int> &, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_zt01> inline constexpr auto key_evaluate<T, double>
#else
inline constexpr auto key_evaluate<T, double, std::enable_if_t<is_same_cvr_v<T, ext_zt01>>>
#endif
    = [](auto &, const symbol_idx_map<double> &, const symbol_set &) constexpr noexcept
{
    return true;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_nzt00> inline constexpr auto key_evaluate<T, int>
#else
inline constexpr auto key_evaluate<T, int, std::enable_if_t<is_same_cvr_v<T, ext_nzt00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return std::string{};
};

} // namespace obake::customisation

TEST_CASE("key_evaluate_test")
{
    REQUIRE(!is_evaluable_key_v<void, void>);
    REQUIRE(!is_evaluable_key_v<ns::zt00, void>);
    REQUIRE(!is_evaluable_key_v<void, ns::zt00>);

    REQUIRE(!is_evaluable_key_v<int, int>);
    REQUIRE(!is_evaluable_key_v<const int &, int>);
    REQUIRE(!is_evaluable_key_v<int &, int>);
    REQUIRE(!is_evaluable_key_v<const int, int>);

    REQUIRE(!is_evaluable_key_v<std::string, std::string>);
    REQUIRE(!is_evaluable_key_v<const std::string &, std::string>);
    REQUIRE(!is_evaluable_key_v<std::string &, std::string>);
    REQUIRE(!is_evaluable_key_v<const std::string, std::string>);

    REQUIRE(is_evaluable_key_v<ns::zt00, int>);
    REQUIRE(is_evaluable_key_v<ns::zt00 &, int>);
    REQUIRE(is_evaluable_key_v<const ns::zt00 &, int>);
    REQUIRE(is_evaluable_key_v<const ns::zt00, int>);
    REQUIRE(!is_evaluable_key_v<const ns::zt00 &, int &>);
    REQUIRE(!is_evaluable_key_v<const ns::zt00 &, const int &>);
    REQUIRE(!is_evaluable_key_v<ns::zt00, double>);

    REQUIRE(!is_evaluable_key_v<ns::zt01, int>);
    REQUIRE(is_evaluable_key_v<ns::zt01 &, int>);
    REQUIRE(!is_evaluable_key_v<const ns::zt01 &, int>);
    REQUIRE(!is_evaluable_key_v<const ns::zt01, int>);
    REQUIRE(!is_evaluable_key_v<const ns::zt01 &, int &>);
    REQUIRE(!is_evaluable_key_v<const ns::zt01 &, const int &>);
    REQUIRE(!is_evaluable_key_v<ns::zt01, double>);

    REQUIRE(!is_evaluable_key_v<ns::nzt00, int>);

    REQUIRE(is_evaluable_key_v<ext_zt00, int>);
    REQUIRE(!is_evaluable_key_v<ext_zt00, double>);
    REQUIRE(is_evaluable_key_v<const ext_zt00, int>);
    REQUIRE(is_evaluable_key_v<const ext_zt00 &, int>);
    REQUIRE(is_evaluable_key_v<ext_zt00 &, int>);

    REQUIRE(!is_evaluable_key_v<ext_zt01, double>);
    REQUIRE(!is_evaluable_key_v<ext_zt01, int>);
    REQUIRE(is_evaluable_key_v<const ext_zt01, double>);
    REQUIRE(is_evaluable_key_v<const ext_zt01 &, double>);
    REQUIRE(is_evaluable_key_v<ext_zt01 &, double>);

    REQUIRE(!is_evaluable_key_v<ext_nzt00, int>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!EvaluableKey<void, void>);
    REQUIRE(!EvaluableKey<ns::zt00, void>);
    REQUIRE(!EvaluableKey<void, ns::zt00>);

    REQUIRE(!EvaluableKey<int, int>);
    REQUIRE(!EvaluableKey<const int &, int>);
    REQUIRE(!EvaluableKey<int &, int>);
    REQUIRE(!EvaluableKey<const int, int>);

    REQUIRE(!EvaluableKey<std::string, std::string>);
    REQUIRE(!EvaluableKey<const std::string &, std::string>);
    REQUIRE(!EvaluableKey<std::string &, std::string>);
    REQUIRE(!EvaluableKey<const std::string, std::string>);

    REQUIRE(EvaluableKey<ns::zt00, int>);
    REQUIRE(EvaluableKey<ns::zt00 &, int>);
    REQUIRE(EvaluableKey<const ns::zt00 &, int>);
    REQUIRE(EvaluableKey<const ns::zt00, int>);
    REQUIRE(!EvaluableKey<const ns::zt00 &, int &>);
    REQUIRE(!EvaluableKey<const ns::zt00 &, const int &>);
    REQUIRE(!EvaluableKey<ns::zt00, double>);

    REQUIRE(!EvaluableKey<ns::zt01, int>);
    REQUIRE(EvaluableKey<ns::zt01 &, int>);
    REQUIRE(!EvaluableKey<const ns::zt01 &, int>);
    REQUIRE(!EvaluableKey<const ns::zt01, int>);
    REQUIRE(!EvaluableKey<const ns::zt01 &, int &>);
    REQUIRE(!EvaluableKey<const ns::zt01 &, const int &>);
    REQUIRE(!EvaluableKey<ns::zt01, double>);

    REQUIRE(!EvaluableKey<ns::nzt00, int>);

    REQUIRE(EvaluableKey<ext_zt00, int>);
    REQUIRE(!EvaluableKey<ext_zt00, double>);
    REQUIRE(EvaluableKey<const ext_zt00, int>);
    REQUIRE(EvaluableKey<const ext_zt00 &, int>);
    REQUIRE(EvaluableKey<ext_zt00 &, int>);

    REQUIRE(!EvaluableKey<ext_zt01, double>);
    REQUIRE(!EvaluableKey<ext_zt01, int>);
    REQUIRE(EvaluableKey<const ext_zt01, double>);
    REQUIRE(EvaluableKey<const ext_zt01 &, double>);
    REQUIRE(EvaluableKey<ext_zt01 &, double>);

    REQUIRE(!EvaluableKey<ext_nzt00, int>);
#endif
}
