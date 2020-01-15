// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>

#include <obake/config.hpp>
#include <obake/key/key_merge_symbols.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

struct no_kms00 {
};

// ADL-based implementations.
namespace ns
{

struct kms00 {
};

kms00 key_merge_symbols(const kms00 &, const symbol_idx_map<symbol_set> &, const symbol_set &);

struct no_kms00 {
};

// Wrong return type.
void key_merge_symbols(const no_kms00 &, const symbol_idx_map<symbol_set> &, const symbol_set &);

struct kms01 {
};

// Only some overloads.
kms01 key_merge_symbols(kms01 &, const symbol_idx_map<symbol_set> &, const symbol_set &);

// Correct, but it will be overridden by an external customisation point.
struct kms02 {
};

kms02 key_merge_symbols(const kms02 &, const symbol_idx_map<symbol_set> &, const symbol_set &);

} // namespace ns

// External customisation point.
struct ext00 {
};

struct ext01 {
};

struct noext00 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext00> inline constexpr auto key_merge_symbols<T>
#else
inline constexpr auto key_merge_symbols<T, std::enable_if_t<is_same_cvr_v<T, ext00>>>
#endif
    = [](auto &&, const symbol_idx_map<symbol_set> &, const symbol_set &) constexpr noexcept
{
    return ext00{};
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ext01> inline constexpr auto key_merge_symbols<T>
#else
inline constexpr auto key_merge_symbols<T, std::enable_if_t<is_same_cvr_v<T, ext01>>>
#endif
    = [](auto &, const symbol_idx_map<symbol_set> &, const symbol_set &) constexpr noexcept
{
    return ext01{};
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, noext00> inline constexpr auto key_merge_symbols<T>
#else
inline constexpr auto key_merge_symbols<T, std::enable_if_t<is_same_cvr_v<T, noext00>>>
#endif
    = [](auto &&, const symbol_idx_map<symbol_set> &, const symbol_set &) constexpr noexcept
{
    return 42;
};

// This will override a correct ADL implementation, thus disabling
// obake::key_merge_symbols().
template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ns::kms02> inline constexpr auto key_merge_symbols<T>
#else
inline constexpr auto key_merge_symbols<T, std::enable_if_t<is_same_cvr_v<T, ns::kms02>>>
#endif
    = [](auto &&, const symbol_idx_map<symbol_set> &, const symbol_set &) constexpr noexcept
{
    return 42;
};

} // namespace obake::customisation

TEST_CASE("key_merge_symbols_test")
{
    REQUIRE(!is_symbols_mergeable_key_v<void>);
    REQUIRE(!is_symbols_mergeable_key_v<int>);
    REQUIRE(!is_symbols_mergeable_key_v<double>);
    REQUIRE(!is_symbols_mergeable_key_v<no_kms00>);

    REQUIRE(is_symbols_mergeable_key_v<ns::kms00>);
    REQUIRE(is_symbols_mergeable_key_v<ns::kms00 &>);
    REQUIRE(is_symbols_mergeable_key_v<const ns::kms00 &>);

    REQUIRE(!is_symbols_mergeable_key_v<ns::no_kms00>);
    REQUIRE(!is_symbols_mergeable_key_v<ns::no_kms00 &>);
    REQUIRE(!is_symbols_mergeable_key_v<const ns::no_kms00 &>);

    REQUIRE(!is_symbols_mergeable_key_v<ns::kms01>);
    REQUIRE(is_symbols_mergeable_key_v<ns::kms01 &>);
    REQUIRE(!is_symbols_mergeable_key_v<const ns::kms01 &>);

    REQUIRE(is_symbols_mergeable_key_v<ext00>);
    REQUIRE(is_symbols_mergeable_key_v<ext00 &>);
    REQUIRE(is_symbols_mergeable_key_v<const ext00 &>);

    REQUIRE(!is_symbols_mergeable_key_v<ext01>);
    REQUIRE(is_symbols_mergeable_key_v<ext01 &>);
    REQUIRE(is_symbols_mergeable_key_v<const ext01 &>);

    REQUIRE(!is_symbols_mergeable_key_v<noext00>);
    REQUIRE(!is_symbols_mergeable_key_v<noext00 &>);
    REQUIRE(!is_symbols_mergeable_key_v<const noext00 &>);

    REQUIRE(!is_symbols_mergeable_key_v<ns::kms02>);
    REQUIRE(!is_symbols_mergeable_key_v<ns::kms02 &>);
    REQUIRE(!is_symbols_mergeable_key_v<const ns::kms02 &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!SymbolsMergeableKey<void>);
    REQUIRE(!SymbolsMergeableKey<int>);
    REQUIRE(!SymbolsMergeableKey<double>);
    REQUIRE(!SymbolsMergeableKey<no_kms00>);

    REQUIRE(SymbolsMergeableKey<ns::kms00>);
    REQUIRE(SymbolsMergeableKey<ns::kms00 &>);
    REQUIRE(SymbolsMergeableKey<const ns::kms00 &>);

    REQUIRE(!SymbolsMergeableKey<ns::no_kms00>);
    REQUIRE(!SymbolsMergeableKey<ns::no_kms00 &>);
    REQUIRE(!SymbolsMergeableKey<const ns::no_kms00 &>);

    REQUIRE(!SymbolsMergeableKey<ns::kms01>);
    REQUIRE(SymbolsMergeableKey<ns::kms01 &>);
    REQUIRE(!SymbolsMergeableKey<const ns::kms01 &>);

    REQUIRE(SymbolsMergeableKey<ext00>);
    REQUIRE(SymbolsMergeableKey<ext00 &>);
    REQUIRE(SymbolsMergeableKey<const ext00 &>);

    REQUIRE(!SymbolsMergeableKey<ext01>);
    REQUIRE(SymbolsMergeableKey<ext01 &>);
    REQUIRE(SymbolsMergeableKey<const ext01 &>);

    REQUIRE(!SymbolsMergeableKey<noext00>);
    REQUIRE(!SymbolsMergeableKey<noext00 &>);
    REQUIRE(!SymbolsMergeableKey<const noext00 &>);

    REQUIRE(!SymbolsMergeableKey<ns::kms02>);
    REQUIRE(!SymbolsMergeableKey<ns::kms02 &>);
    REQUIRE(!SymbolsMergeableKey<const ns::kms02 &>);
#endif
}
