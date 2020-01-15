// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>
#include <type_traits>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#if defined(MPPP_WITH_MPFR)

#include <mp++/real.hpp>

#endif

#if defined(MPPP_WITH_QUADMATH)

#include <mp++/real128.hpp>

#endif

#include <obake/config.hpp>
#include <obake/math/diff.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("diff_test")
{
    REQUIRE(!is_differentiable_v<void>);
    REQUIRE(!is_differentiable_v<const void>);

    REQUIRE(!is_differentiable_v<std::string>);
    REQUIRE(!is_differentiable_v<std::string &>);
    REQUIRE(!is_differentiable_v<const std::string &>);
    REQUIRE(!is_differentiable_v<const std::string>);

    REQUIRE(is_differentiable_v<int>);
    REQUIRE(is_differentiable_v<int &>);
    REQUIRE(is_differentiable_v<const int>);
    REQUIRE(is_differentiable_v<const int &>);

    REQUIRE(is_differentiable_v<double>);
    REQUIRE(is_differentiable_v<double &>);
    REQUIRE(is_differentiable_v<const double>);
    REQUIRE(is_differentiable_v<const double &>);

#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(is_differentiable_v<__int128_t>);
    REQUIRE(is_differentiable_v<__int128_t &>);
    REQUIRE(is_differentiable_v<const __int128_t>);
    REQUIRE(is_differentiable_v<const __int128_t &>);

    REQUIRE(is_differentiable_v<__uint128_t>);
    REQUIRE(is_differentiable_v<__uint128_t &>);
    REQUIRE(is_differentiable_v<const __uint128_t>);
    REQUIRE(is_differentiable_v<const __uint128_t &>);
#endif

    REQUIRE(is_differentiable_v<mppp::integer<1>>);
    REQUIRE(is_differentiable_v<mppp::integer<1> &>);
    REQUIRE(is_differentiable_v<const mppp::integer<1> &>);
    REQUIRE(is_differentiable_v<const mppp::integer<1>>);

    REQUIRE(is_differentiable_v<mppp::rational<1>>);
    REQUIRE(is_differentiable_v<mppp::rational<1> &>);
    REQUIRE(is_differentiable_v<const mppp::rational<1> &>);
    REQUIRE(is_differentiable_v<const mppp::rational<1>>);

#if defined(MPPP_WITH_MPFR)
    REQUIRE(is_differentiable_v<mppp::real>);
    REQUIRE(is_differentiable_v<mppp::real &>);
    REQUIRE(is_differentiable_v<const mppp::real &>);
    REQUIRE(is_differentiable_v<const mppp::real>);
#endif

#if defined(MPPP_WITH_QUADMATH)
    REQUIRE(is_differentiable_v<mppp::real128>);
    REQUIRE(is_differentiable_v<mppp::real128 &>);
    REQUIRE(is_differentiable_v<const mppp::real128 &>);
    REQUIRE(is_differentiable_v<const mppp::real128>);
#endif

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!Differentiable<void>);
    REQUIRE(!Differentiable<const void>);

    REQUIRE(!Differentiable<std::string>);
    REQUIRE(!Differentiable<std::string &>);
    REQUIRE(!Differentiable<const std::string &>);
    REQUIRE(!Differentiable<const std::string>);

    REQUIRE(Differentiable<int>);
    REQUIRE(Differentiable<int &>);
    REQUIRE(Differentiable<const int>);
    REQUIRE(Differentiable<const int &>);

    REQUIRE(Differentiable<double>);
    REQUIRE(Differentiable<double &>);
    REQUIRE(Differentiable<const double>);
    REQUIRE(Differentiable<const double &>);

#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(Differentiable<__int128_t>);
    REQUIRE(Differentiable<__int128_t &>);
    REQUIRE(Differentiable<const __int128_t>);
    REQUIRE(Differentiable<const __int128_t &>);

    REQUIRE(Differentiable<__uint128_t>);
    REQUIRE(Differentiable<__uint128_t &>);
    REQUIRE(Differentiable<const __uint128_t>);
    REQUIRE(Differentiable<const __uint128_t &>);
#endif

    REQUIRE(Differentiable<mppp::integer<1>>);
    REQUIRE(Differentiable<mppp::integer<1> &>);
    REQUIRE(Differentiable<const mppp::integer<1> &>);
    REQUIRE(Differentiable<const mppp::integer<1>>);

    REQUIRE(Differentiable<mppp::rational<1>>);
    REQUIRE(Differentiable<mppp::rational<1> &>);
    REQUIRE(Differentiable<const mppp::rational<1> &>);
    REQUIRE(Differentiable<const mppp::rational<1>>);

#if defined(MPPP_WITH_MPFR)
    REQUIRE(Differentiable<mppp::real>);
    REQUIRE(Differentiable<mppp::real &>);
    REQUIRE(Differentiable<const mppp::real &>);
    REQUIRE(Differentiable<const mppp::real>);
#endif

#if defined(MPPP_WITH_QUADMATH)
    REQUIRE(Differentiable<mppp::real128>);
    REQUIRE(Differentiable<mppp::real128 &>);
    REQUIRE(Differentiable<const mppp::real128 &>);
    REQUIRE(Differentiable<const mppp::real128>);
#endif
#endif

    REQUIRE(diff(0, "") == 0);
    REQUIRE(diff(1, "x") == 0);
    REQUIRE(diff(2., "x") == 0.);
    REQUIRE(diff(2.f, "x") == 0.f);
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(diff(__int128_t(1), "x") == 0);
    REQUIRE(diff(__uint128_t(1), "x") == 0);
#endif
    REQUIRE(std::is_same_v<int, decltype(diff(0, ""))>);
    REQUIRE(std::is_same_v<double, decltype(diff(0., ""))>);

    REQUIRE(diff(mppp::integer<1>{4}, "") == 0);
    REQUIRE(diff(mppp::rational<1>{4, 3}, "") == 0);
    REQUIRE(std::is_same_v<mppp::integer<1>, decltype(diff(mppp::integer<1>{4}, ""))>);
    REQUIRE(std::is_same_v<mppp::rational<1>, decltype(diff(mppp::rational<1>{4, 3}, ""))>);

#if defined(MPPP_WITH_MPFR)
    REQUIRE(diff(mppp::real{4}, "") == 0);
    REQUIRE(std::is_same_v<mppp::real, decltype(diff(mppp::real{4}, ""))>);
    REQUIRE(diff(mppp::real{4, 135}, "").get_prec() == 135);
    REQUIRE(diff(mppp::real{4, 135}, "") == 0);
#endif

#if defined(MPPP_WITH_QUADMATH)
    REQUIRE(diff(mppp::real128{4}, "") == 0);
    REQUIRE(std::is_same_v<mppp::real128, decltype(diff(mppp::real128{4}, ""))>);
#endif
}

struct nodiff_00 {
};

namespace ns
{

struct diff_00 {
};

diff_00 diff(const diff_00 &, const std::string &);

struct diff_01 {
};

diff_01 diff(diff_01 &, const std::string &);

struct diff_02 {
};

// Wrong signature.
void diff(const diff_02 &);

} // namespace ns

struct diff_ext {
};

struct nodiff_ext_00 {
};

struct nodiff_ext_01 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, diff_ext> inline constexpr auto diff<T>
#else
inline constexpr auto diff<T, std::enable_if_t<is_same_cvr_v<T, diff_ext>>>
#endif
    = [](auto &&, const std::string &) constexpr noexcept
{
    return diff_ext{};
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, nodiff_ext_00> inline constexpr auto diff<T>
#else
inline constexpr auto diff<T, std::enable_if_t<is_same_cvr_v<T, nodiff_ext_00>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return 1;
};

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, nodiff_ext_01> inline constexpr auto diff<T>
#else
inline constexpr auto diff<T, std::enable_if_t<is_same_cvr_v<T, nodiff_ext_01>>>
#endif
    = [](nodiff_ext_01 &, const std::string &) constexpr noexcept
{
    return nodiff_ext_01{};
};

} // namespace obake::customisation

TEST_CASE("diff_custom_test")
{
    REQUIRE(is_differentiable_v<diff_ext>);
    REQUIRE(is_differentiable_v<diff_ext &>);
    REQUIRE(is_differentiable_v<const diff_ext &>);
    REQUIRE(is_differentiable_v<const diff_ext>);

    REQUIRE(!is_differentiable_v<nodiff_ext_00>);
    REQUIRE(!is_differentiable_v<nodiff_ext_00 &>);
    REQUIRE(!is_differentiable_v<const nodiff_ext_00 &>);
    REQUIRE(!is_differentiable_v<const nodiff_ext_00>);

    REQUIRE(!is_differentiable_v<nodiff_ext_01>);
    REQUIRE(is_differentiable_v<nodiff_ext_01 &>);
    REQUIRE(!is_differentiable_v<const nodiff_ext_01 &>);
    REQUIRE(!is_differentiable_v<const nodiff_ext_01>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(Differentiable<diff_ext>);
    REQUIRE(Differentiable<diff_ext &>);
    REQUIRE(Differentiable<const diff_ext &>);
    REQUIRE(Differentiable<const diff_ext>);

    REQUIRE(!Differentiable<nodiff_ext_00>);
    REQUIRE(!Differentiable<nodiff_ext_00 &>);
    REQUIRE(!Differentiable<const nodiff_ext_00 &>);
    REQUIRE(!Differentiable<const nodiff_ext_00>);

    REQUIRE(!Differentiable<nodiff_ext_01>);
    REQUIRE(Differentiable<nodiff_ext_01 &>);
    REQUIRE(!Differentiable<const nodiff_ext_01 &>);
    REQUIRE(!Differentiable<const nodiff_ext_01>);
#endif
}
