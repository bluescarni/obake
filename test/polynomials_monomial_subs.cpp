// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>
#include <type_traits>
#include <utility>

#include <obake/config.hpp>
#include <obake/polynomials/monomial_subs.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

// ADL-based implementations.
namespace ns
{

struct zt00 {
};

std::pair<bool, zt00> monomial_subs(const zt00 &, const symbol_idx_map<int> &, const symbol_set &);

struct zt01 {
};

// Disable certain overloads.
std::pair<bool, zt01> monomial_subs(zt01 &, const symbol_idx_map<int> &, const symbol_set &);

struct nzt00 {
};

// Wrong signature type.
std::string monomial_subs(nzt00 &, const symbol_set &);

struct nzt01 {
};

// Another wrong signature type.
std::pair<bool, nzt01 &> monomial_subs(const nzt01 &, const symbol_idx_map<int> &, const symbol_set &);

struct nzt02 {
};

// Another wrong signature type.
std::pair<bool, const nzt02> monomial_subs(const nzt02 &, const symbol_idx_map<int> &, const symbol_set &);

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
requires SameCvr<T, ext_zt00> inline constexpr auto monomial_subs<T, int>
#else
inline constexpr auto monomial_subs<T, int, std::enable_if_t<is_same_cvr_v<T, ext_zt00>>>
#endif
    = [](auto &&, const symbol_idx_map<int> &, const symbol_set &) constexpr noexcept
{
    return std::make_pair(true, ext_zt00{});
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_zt01> inline constexpr auto monomial_subs<T, double>
#else
inline constexpr auto monomial_subs<T, double, std::enable_if_t<is_same_cvr_v<T, ext_zt01>>>
#endif
    = [](auto &, const symbol_idx_map<double> &, const symbol_set &) constexpr noexcept
{
    return std::make_pair(true, ext_zt01{});
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext_nzt00> inline constexpr auto monomial_subs<T, int>
#else
inline constexpr auto monomial_subs<T, int, std::enable_if_t<is_same_cvr_v<T, ext_nzt00>>>
#endif
    = [](auto &&, const symbol_set &) constexpr noexcept
{
    return std::string{};
};

} // namespace obake::customisation

TEST_CASE("monomial_subs_test")
{
    REQUIRE(!is_substitutable_monomial_v<void, void>);
    REQUIRE(!is_substitutable_monomial_v<ns::zt00, void>);
    REQUIRE(!is_substitutable_monomial_v<void, ns::zt00>);

    REQUIRE(!is_substitutable_monomial_v<int, int>);
    REQUIRE(!is_substitutable_monomial_v<const int &, int>);
    REQUIRE(!is_substitutable_monomial_v<int &, int>);
    REQUIRE(!is_substitutable_monomial_v<const int, int>);

    REQUIRE(!is_substitutable_monomial_v<std::string, std::string>);
    REQUIRE(!is_substitutable_monomial_v<const std::string &, std::string>);
    REQUIRE(!is_substitutable_monomial_v<std::string &, std::string>);
    REQUIRE(!is_substitutable_monomial_v<const std::string, std::string>);

    REQUIRE(is_substitutable_monomial_v<ns::zt00, int>);
    REQUIRE(is_substitutable_monomial_v<ns::zt00 &, int>);
    REQUIRE(is_substitutable_monomial_v<const ns::zt00 &, int>);
    REQUIRE(is_substitutable_monomial_v<const ns::zt00, int>);
    REQUIRE(!is_substitutable_monomial_v<const ns::zt00 &, int &>);
    REQUIRE(!is_substitutable_monomial_v<const ns::zt00 &, const int &>);
    REQUIRE(!is_substitutable_monomial_v<ns::zt00, double>);

    REQUIRE(!is_substitutable_monomial_v<ns::zt01, int>);
    REQUIRE(is_substitutable_monomial_v<ns::zt01 &, int>);
    REQUIRE(!is_substitutable_monomial_v<const ns::zt01 &, int>);
    REQUIRE(!is_substitutable_monomial_v<const ns::zt01, int>);
    REQUIRE(!is_substitutable_monomial_v<const ns::zt01 &, int &>);
    REQUIRE(!is_substitutable_monomial_v<const ns::zt01 &, const int &>);
    REQUIRE(!is_substitutable_monomial_v<ns::zt01, double>);

    REQUIRE(!is_substitutable_monomial_v<ns::nzt00, int>);
    REQUIRE(!is_substitutable_monomial_v<ns::nzt01, int>);
    REQUIRE(!is_substitutable_monomial_v<ns::nzt02, int>);

    REQUIRE(is_substitutable_monomial_v<ext_zt00, int>);
    REQUIRE(!is_substitutable_monomial_v<ext_zt00, double>);
    REQUIRE(is_substitutable_monomial_v<const ext_zt00, int>);
    REQUIRE(is_substitutable_monomial_v<const ext_zt00 &, int>);
    REQUIRE(is_substitutable_monomial_v<ext_zt00 &, int>);

    REQUIRE(!is_substitutable_monomial_v<ext_zt01, double>);
    REQUIRE(!is_substitutable_monomial_v<ext_zt01, int>);
    REQUIRE(is_substitutable_monomial_v<const ext_zt01, double>);
    REQUIRE(is_substitutable_monomial_v<const ext_zt01 &, double>);
    REQUIRE(is_substitutable_monomial_v<ext_zt01 &, double>);

    REQUIRE(!is_substitutable_monomial_v<ext_nzt00, int>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!SubstitutableMonomial<void, void>);
    REQUIRE(!SubstitutableMonomial<ns::zt00, void>);
    REQUIRE(!SubstitutableMonomial<void, ns::zt00>);

    REQUIRE(!SubstitutableMonomial<int, int>);
    REQUIRE(!SubstitutableMonomial<const int &, int>);
    REQUIRE(!SubstitutableMonomial<int &, int>);
    REQUIRE(!SubstitutableMonomial<const int, int>);

    REQUIRE(!SubstitutableMonomial<std::string, std::string>);
    REQUIRE(!SubstitutableMonomial<const std::string &, std::string>);
    REQUIRE(!SubstitutableMonomial<std::string &, std::string>);
    REQUIRE(!SubstitutableMonomial<const std::string, std::string>);

    REQUIRE(SubstitutableMonomial<ns::zt00, int>);
    REQUIRE(SubstitutableMonomial<ns::zt00 &, int>);
    REQUIRE(SubstitutableMonomial<const ns::zt00 &, int>);
    REQUIRE(SubstitutableMonomial<const ns::zt00, int>);
    REQUIRE(!SubstitutableMonomial<const ns::zt00 &, int &>);
    REQUIRE(!SubstitutableMonomial<const ns::zt00 &, const int &>);
    REQUIRE(!SubstitutableMonomial<ns::zt00, double>);

    REQUIRE(!SubstitutableMonomial<ns::zt01, int>);
    REQUIRE(SubstitutableMonomial<ns::zt01 &, int>);
    REQUIRE(!SubstitutableMonomial<const ns::zt01 &, int>);
    REQUIRE(!SubstitutableMonomial<const ns::zt01, int>);
    REQUIRE(!SubstitutableMonomial<const ns::zt01 &, int &>);
    REQUIRE(!SubstitutableMonomial<const ns::zt01 &, const int &>);
    REQUIRE(!SubstitutableMonomial<ns::zt01, double>);

    REQUIRE(!SubstitutableMonomial<ns::nzt00, int>);
    REQUIRE(!SubstitutableMonomial<ns::nzt01, int>);
    REQUIRE(!SubstitutableMonomial<ns::nzt02, int>);

    REQUIRE(SubstitutableMonomial<ext_zt00, int>);
    REQUIRE(!SubstitutableMonomial<ext_zt00, double>);
    REQUIRE(SubstitutableMonomial<const ext_zt00, int>);
    REQUIRE(SubstitutableMonomial<const ext_zt00 &, int>);
    REQUIRE(SubstitutableMonomial<ext_zt00 &, int>);

    REQUIRE(!SubstitutableMonomial<ext_zt01, double>);
    REQUIRE(!SubstitutableMonomial<ext_zt01, int>);
    REQUIRE(SubstitutableMonomial<const ext_zt01, double>);
    REQUIRE(SubstitutableMonomial<const ext_zt01 &, double>);
    REQUIRE(SubstitutableMonomial<ext_zt01 &, double>);

    REQUIRE(!SubstitutableMonomial<ext_nzt00, int>);
#endif
}
