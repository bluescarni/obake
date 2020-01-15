// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>
#include <utility>

#include <obake/config.hpp>
#include <obake/polynomials/monomial_diff.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

// ADL-based implementations.
namespace ns
{

struct zt00 {
};

std::pair<bool, zt00> monomial_diff(const zt00 &, const symbol_idx &, const symbol_set &);

struct zt01 {
};

// Disable certain overloads.
std::pair<bool, zt01> monomial_diff(zt01 &, const symbol_idx &, const symbol_set &);

struct nzt00 {
};

// Wrong signature type.
std::string monomial_diff(nzt00 &, const symbol_set &);

struct nzt01 {
};

// Another wrong signature type.
std::pair<bool, nzt01 &> monomial_diff(const nzt01 &, const symbol_idx &, const symbol_set &);

struct nzt02 {
};

// Another wrong signature type.
std::pair<bool, const nzt02> monomial_diff(const nzt02 &, const symbol_idx &, const symbol_set &);

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
requires SameCvr<T, ext_zt00> inline constexpr auto monomial_diff<T>
#else
inline constexpr auto monomial_diff<T, std::enable_if_t<is_same_cvr_v<T, ext_zt00>>>
#endif
    = [](auto &&, const symbol_idx &, const symbol_set &) constexpr noexcept
{
    return std::make_pair(true, ext_zt00{});
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_zt01> inline constexpr auto monomial_diff<T>
#else
inline constexpr auto monomial_diff<T, std::enable_if_t<is_same_cvr_v<T, ext_zt01>>>
#endif
    = [](auto &, const symbol_idx &, const symbol_set &) constexpr noexcept
{
    return std::make_pair(true, ext_zt01{});
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_nzt00> inline constexpr auto monomial_diff<T>
#else
inline constexpr auto monomial_diff<T, std::enable_if_t<is_same_cvr_v<T, ext_nzt00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return std::string{};
};

} // namespace obake::customisation

TEST_CASE("monomial_diff_test")
{
    REQUIRE(!is_differentiable_monomial_v<void>);
    REQUIRE(!is_differentiable_monomial_v<const void>);
    REQUIRE(!is_differentiable_monomial_v<int>);
    REQUIRE(!is_differentiable_monomial_v<int &>);
    REQUIRE(!is_differentiable_monomial_v<const int &>);
    REQUIRE(!is_differentiable_monomial_v<const int>);

    REQUIRE(is_differentiable_monomial_v<ns::zt00>);
    REQUIRE(is_differentiable_monomial_v<ns::zt00 &>);
    REQUIRE(is_differentiable_monomial_v<const ns::zt00 &>);
    REQUIRE(is_differentiable_monomial_v<const ns::zt00>);

    REQUIRE(!is_differentiable_monomial_v<ns::zt01>);
    REQUIRE(is_differentiable_monomial_v<ns::zt01 &>);
    REQUIRE(!is_differentiable_monomial_v<const ns::zt01 &>);
    REQUIRE(!is_differentiable_monomial_v<const ns::zt01>);

    REQUIRE(is_differentiable_monomial_v<ext_zt00>);
    REQUIRE(is_differentiable_monomial_v<const ext_zt00>);
    REQUIRE(is_differentiable_monomial_v<const ext_zt00 &>);
    REQUIRE(is_differentiable_monomial_v<ext_zt00 &>);

    REQUIRE(!is_differentiable_monomial_v<ext_zt01>);
    REQUIRE(is_differentiable_monomial_v<const ext_zt01>);
    REQUIRE(is_differentiable_monomial_v<const ext_zt01 &>);
    REQUIRE(is_differentiable_monomial_v<ext_zt01 &>);

    REQUIRE(!is_differentiable_monomial_v<ext_nzt00>);
    REQUIRE(!is_differentiable_monomial_v<ext_nzt00 &>);
    REQUIRE(!is_differentiable_monomial_v<const ext_nzt00 &>);
    REQUIRE(!is_differentiable_monomial_v<const ext_nzt00>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!DifferentiableMonomial<void>);
    REQUIRE(!DifferentiableMonomial<const void>);
    REQUIRE(!DifferentiableMonomial<int>);
    REQUIRE(!DifferentiableMonomial<int &>);
    REQUIRE(!DifferentiableMonomial<const int &>);
    REQUIRE(!DifferentiableMonomial<const int>);

    REQUIRE(DifferentiableMonomial<ns::zt00>);
    REQUIRE(DifferentiableMonomial<ns::zt00 &>);
    REQUIRE(DifferentiableMonomial<const ns::zt00 &>);
    REQUIRE(DifferentiableMonomial<const ns::zt00>);

    REQUIRE(!DifferentiableMonomial<ns::zt01>);
    REQUIRE(DifferentiableMonomial<ns::zt01 &>);
    REQUIRE(!DifferentiableMonomial<const ns::zt01 &>);
    REQUIRE(!DifferentiableMonomial<const ns::zt01>);

    REQUIRE(DifferentiableMonomial<ext_zt00>);
    REQUIRE(DifferentiableMonomial<const ext_zt00>);
    REQUIRE(DifferentiableMonomial<const ext_zt00 &>);
    REQUIRE(DifferentiableMonomial<ext_zt00 &>);

    REQUIRE(!DifferentiableMonomial<ext_zt01>);
    REQUIRE(DifferentiableMonomial<const ext_zt01>);
    REQUIRE(DifferentiableMonomial<const ext_zt01 &>);
    REQUIRE(DifferentiableMonomial<ext_zt01 &>);

    REQUIRE(!DifferentiableMonomial<ext_nzt00>);
    REQUIRE(!DifferentiableMonomial<ext_nzt00 &>);
    REQUIRE(!DifferentiableMonomial<const ext_nzt00 &>);
    REQUIRE(!DifferentiableMonomial<const ext_nzt00>);
#endif
}
