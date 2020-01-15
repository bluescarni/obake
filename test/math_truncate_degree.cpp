// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>

#include <obake/config.hpp>
#include <obake/math/truncate_degree.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

#if defined(OBAKE_COMPILER_IS_GCC) && __GNUC__ == 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

TEST_CASE("truncate_degree_arith")
{
    // Check type-traits/concepts.
    REQUIRE(!is_degree_truncatable_v<void, void>);
    REQUIRE(!is_degree_truncatable_v<void, int>);
    REQUIRE(!is_degree_truncatable_v<int, void>);
    REQUIRE(!is_degree_truncatable_v<int, int>);
    REQUIRE(!is_degree_truncatable_v<int &, const int>);
    REQUIRE(!is_degree_truncatable_v<const int &, double>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!DegreeTruncatable<void, void>);
    REQUIRE(!DegreeTruncatable<void, int>);
    REQUIRE(!DegreeTruncatable<int, void>);
    REQUIRE(!DegreeTruncatable<int, int>);
    REQUIRE(!DegreeTruncatable<int &, const int>);
    REQUIRE(!DegreeTruncatable<const int &, double>);
#endif
}

struct notr_00 {
};

namespace ns
{

struct tr_00 {
};

void truncate_degree(tr_00 &, int);
void truncate_degree(tr_00 &&, int);

struct tr_01 {
};

// Enable only certain overloads.
void truncate_degree(tr_01 &, int);

struct tr_03 {
};

// Wrong signature.
int truncate_degree(const tr_03 &);

} // namespace ns

struct tr_ext {
};

struct notr_ext_01 {
};

namespace obake::customisation
{

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, tr_ext> inline constexpr auto truncate_degree<T, U>
#else
inline constexpr auto truncate_degree<T, U, std::enable_if_t<is_same_cvr_v<T, tr_ext>>>
#endif
    = [](auto &&, auto &&) constexpr noexcept
{
    return tr_ext{};
};

// Wrong signature.
template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, notr_ext_01> inline constexpr auto truncate_degree<T, U>
#else
inline constexpr auto truncate_degree<T, U, std::enable_if_t<is_same_cvr_v<T, notr_ext_01>>>
#endif
    = [](notr_ext_01 &) constexpr noexcept
{
    return 1;
};

} // namespace obake::customisation

TEST_CASE("truncate_degree_custom")
{
    REQUIRE(!is_degree_truncatable_v<ns::tr_00, void>);
    REQUIRE(!is_degree_truncatable_v<ns::tr_00 &, void>);
    REQUIRE(!is_degree_truncatable_v<const ns::tr_00 &, void>);
    REQUIRE(!is_degree_truncatable_v<const ns::tr_00, void>);

    REQUIRE(is_degree_truncatable_v<ns::tr_00, int>);
    REQUIRE(is_degree_truncatable_v<ns::tr_00 &, int &>);
    REQUIRE(!is_degree_truncatable_v<const ns::tr_00 &, const int>);
    REQUIRE(!is_degree_truncatable_v<const ns::tr_00 &, int &>);
    REQUIRE(is_degree_truncatable_v<ns::tr_00, double>);

    REQUIRE(!is_degree_truncatable_v<ns::tr_01, int>);
    REQUIRE(is_degree_truncatable_v<ns::tr_01 &, int &>);
    REQUIRE(!is_degree_truncatable_v<const ns::tr_01 &, const int>);
    REQUIRE(!is_degree_truncatable_v<const ns::tr_01 &, int &>);
    REQUIRE(!is_degree_truncatable_v<ns::tr_01, double>);

    REQUIRE(!is_degree_truncatable_v<ns::tr_03, int>);

    REQUIRE(is_degree_truncatable_v<tr_ext, int>);
    REQUIRE(is_degree_truncatable_v<tr_ext &, int &>);
    REQUIRE(is_degree_truncatable_v<const tr_ext &, const int>);
    REQUIRE(is_degree_truncatable_v<const tr_ext &, int &>);
    REQUIRE(is_degree_truncatable_v<tr_ext, double>);

    REQUIRE(!is_degree_truncatable_v<notr_ext_01, int>);
    REQUIRE(!is_degree_truncatable_v<notr_ext_01 &, int &>);
    REQUIRE(!is_degree_truncatable_v<const notr_ext_01 &, const int>);
    REQUIRE(!is_degree_truncatable_v<const notr_ext_01 &, int &>);
    REQUIRE(!is_degree_truncatable_v<notr_ext_01, double>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!DegreeTruncatable<ns::tr_00, void>);
    REQUIRE(!DegreeTruncatable<ns::tr_00 &, void>);
    REQUIRE(!DegreeTruncatable<const ns::tr_00 &, void>);
    REQUIRE(!DegreeTruncatable<const ns::tr_00, void>);

    REQUIRE(DegreeTruncatable<ns::tr_00, int>);
    REQUIRE(DegreeTruncatable<ns::tr_00 &, int &>);
    REQUIRE(!DegreeTruncatable<const ns::tr_00 &, const int>);
    REQUIRE(!DegreeTruncatable<const ns::tr_00 &, int &>);
    REQUIRE(DegreeTruncatable<ns::tr_00, double>);

    REQUIRE(!DegreeTruncatable<ns::tr_01, int>);
    REQUIRE(DegreeTruncatable<ns::tr_01 &, int &>);
    REQUIRE(!DegreeTruncatable<const ns::tr_01 &, const int>);
    REQUIRE(!DegreeTruncatable<const ns::tr_01 &, int &>);
    REQUIRE(!DegreeTruncatable<ns::tr_01, double>);

    REQUIRE(!DegreeTruncatable<ns::tr_03, int>);

    REQUIRE(DegreeTruncatable<tr_ext, int>);
    REQUIRE(DegreeTruncatable<tr_ext &, int &>);
    REQUIRE(DegreeTruncatable<const tr_ext &, const int>);
    REQUIRE(DegreeTruncatable<const tr_ext &, int &>);
    REQUIRE(DegreeTruncatable<tr_ext, double>);

    REQUIRE(!DegreeTruncatable<notr_ext_01, int>);
    REQUIRE(!DegreeTruncatable<notr_ext_01 &, int &>);
    REQUIRE(!DegreeTruncatable<const notr_ext_01 &, const int>);
    REQUIRE(!DegreeTruncatable<const notr_ext_01 &, int &>);
    REQUIRE(!DegreeTruncatable<notr_ext_01, double>);
#endif
}

#if defined(OBAKE_COMPILER_IS_GCC) && __GNUC__ == 8
#pragma GCC diagnostic pop
#endif
