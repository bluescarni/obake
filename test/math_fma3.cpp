// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cmath>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>

#if defined(MPPP_WITH_MPFR)

#include <mp++/real.hpp>

#endif

#include <obake/config.hpp>
#include <obake/math/fma3.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("fma3_fp")
{
#if defined(FP_FAST_FMAF)
    auto xf = 1.f;
    obake::fma3(xf, 3.f, 4.f);
    REQUIRE(xf == 13.f);
#else
    REQUIRE(!is_mult_addable_v<float &, const float &, const float &>);
#endif

#if defined(FP_FAST_FMA)
    auto xd = 1.;
    obake::fma3(xd, 3., 4.);
    REQUIRE(xd == 13.);
    REQUIRE(std::is_same_v<void, decltype(obake::fma3(xd, 3., 4.))>);
#else
    REQUIRE(!is_mult_addable_v<double &, const double &, const double &>);
#endif

#if defined(FP_FAST_FMAL)
    auto xld = 1.l;
    obake::fma3(xld, 3.l, 4.l);
    REQUIRE(xld == 13.l);
    REQUIRE(std::is_same_v<void, decltype(obake::fma3(xld, 3.l, 4.l))>);
#else
    REQUIRE(!is_mult_addable_v<long double &, const long double &, const long double &>);
#endif

    REQUIRE(!is_mult_addable_v<const float &, const float &, const float &>);
    REQUIRE(!is_mult_addable_v<const double &, const double &, const double &>);
    REQUIRE(!is_mult_addable_v<const long double &, const long double &, const long double &>);

    REQUIRE(!is_mult_addable_v<void, void, void>);
    REQUIRE(!is_mult_addable_v<float &, void, void>);
    REQUIRE(!is_mult_addable_v<void, const float &, void>);
    REQUIRE(!is_mult_addable_v<void, void, const float &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!MultAddable<const float &, const float &, const float &>);
    REQUIRE(!MultAddable<const double &, const double &, const double &>);
    REQUIRE(!MultAddable<const long double &, const long double &, const long double &>);

    REQUIRE(!MultAddable<void, void, void>);
    REQUIRE(!MultAddable<float &, void, void>);
    REQUIRE(!MultAddable<void, const float &, void>);
    REQUIRE(!MultAddable<void, void, const float &>);
#endif
}

TEST_CASE("fma3_integral")
{
    REQUIRE(!is_mult_addable_v<int &, const int &, const int &>);
    REQUIRE(!is_mult_addable_v<const int &, const int &, const int &>);

    REQUIRE(!is_mult_addable_v<void, void, void>);
    REQUIRE(!is_mult_addable_v<int &, void, void>);
    REQUIRE(!is_mult_addable_v<void, const int &, void>);
    REQUIRE(!is_mult_addable_v<void, void, const int &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!MultAddable<const int &, const int &, const int &>);

    REQUIRE(!MultAddable<void, void, void>);
    REQUIRE(!MultAddable<int &, void, void>);
    REQUIRE(!MultAddable<void, const int &, void>);
    REQUIRE(!MultAddable<void, void, const int &>);
#endif
}

TEST_CASE("fma3_mp++_integer")
{
    using int_t = mppp::integer<1>;

    int_t n{5};
    obake::fma3(n, int_t{6}, int_t{7});
    REQUIRE(n == 47);

    REQUIRE(!is_mult_addable_v<const int_t &, const int_t &, const int_t &>);

    REQUIRE(!is_mult_addable_v<void, void, void>);
    REQUIRE(!is_mult_addable_v<int_t &, void, void>);
    REQUIRE(!is_mult_addable_v<void, const int_t &, void>);
    REQUIRE(!is_mult_addable_v<void, void, const int_t &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!MultAddable<const int_t &, const int_t &, const int_t &>);

    REQUIRE(!MultAddable<void, void, void>);
    REQUIRE(!MultAddable<int_t &, void, void>);
    REQUIRE(!MultAddable<void, const int_t &, void>);
    REQUIRE(!MultAddable<void, void, const int_t &>);
#endif
}

#if defined(MPPP_WITH_MPFR)

TEST_CASE("fma3_mp++_real")
{
    mppp::real x{5};
    obake::fma3(x, mppp::real{6}, mppp::real{7});
    REQUIRE(x == 47);

    REQUIRE(!is_mult_addable_v<const mppp::real &, const mppp::real &, const mppp::real &>);

    REQUIRE(!is_mult_addable_v<void, void, void>);
    REQUIRE(!is_mult_addable_v<mppp::real &, void, void>);
    REQUIRE(!is_mult_addable_v<void, const mppp::real &, void>);
    REQUIRE(!is_mult_addable_v<void, void, const mppp::real &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!MultAddable<const mppp::real &, const mppp::real &, const mppp::real &>);

    REQUIRE(!MultAddable<void, void, void>);
    REQUIRE(!MultAddable<mppp::real &, void, void>);
    REQUIRE(!MultAddable<void, const mppp::real &, void>);
    REQUIRE(!MultAddable<void, void, const mppp::real &>);
#endif
}

#endif

#if defined(MPPP_WITH_QUADMATH)

TEST_CASE("fma3_mp++_real128")
{
    mppp::real128 x{5};
    obake::fma3(x, mppp::real128{6}, mppp::real128{7});
    REQUIRE(x == 47);

    REQUIRE(!is_mult_addable_v<const mppp::real128 &, const mppp::real128 &, const mppp::real128 &>);

    REQUIRE(!is_mult_addable_v<void, void, void>);
    REQUIRE(!is_mult_addable_v<mppp::real128 &, void, void>);
    REQUIRE(!is_mult_addable_v<void, const mppp::real128 &, void>);
    REQUIRE(!is_mult_addable_v<void, void, const mppp::real128 &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!MultAddable<const mppp::real128 &, const mppp::real128 &, const mppp::real128 &>);

    REQUIRE(!MultAddable<void, void, void>);
    REQUIRE(!MultAddable<mppp::real128 &, void, void>);
    REQUIRE(!MultAddable<void, const mppp::real128 &, void>);
    REQUIRE(!MultAddable<void, void, const mppp::real128 &>);
#endif
}

#endif

namespace ns
{

// ADL-based customisation.
struct foo {
};

int fma3(foo &, const foo &, const foo &);

} // namespace ns

// External customisation point.
struct bar {
};

struct nobar {
};

namespace obake::customisation
{

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, bar> &&SameCvr<U, bar> inline constexpr auto fma3<bar &, T, U>
#else
inline constexpr auto fma3<bar &, T, U, std::enable_if_t<std::conjunction_v<is_same_cvr<T, bar>, is_same_cvr<U, bar>>>>
#endif
    = [](auto &, auto &&, auto &&) constexpr noexcept
{
    return true;
};

} // namespace obake::customisation

TEST_CASE("fma3_custom")
{
    REQUIRE(is_mult_addable_v<ns::foo &, const ns::foo &, const ns::foo &>);
    REQUIRE(!is_mult_addable_v<const ns::foo &, const ns::foo &, const ns::foo &>);
    REQUIRE(!is_mult_addable_v<ns::foo &&, const ns::foo &, const ns::foo &>);

    REQUIRE(is_mult_addable_v<bar &, const bar &, const bar &>);
    REQUIRE(!is_mult_addable_v<const bar &, const bar &, const bar &>);
    REQUIRE(!is_mult_addable_v<bar &&, const bar &, const bar &>);

    REQUIRE(!is_mult_addable_v<nobar &, const nobar &, const nobar &>);
    REQUIRE(!is_mult_addable_v<const nobar &, const nobar &, const nobar &>);
    REQUIRE(!is_mult_addable_v<nobar &&, const nobar &, const nobar &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(MultAddable<ns::foo &, const ns::foo &, const ns::foo &>);
    REQUIRE(!MultAddable<const ns::foo &, const ns::foo &, const ns::foo &>);
    REQUIRE(!MultAddable<ns::foo &&, const ns::foo &, const ns::foo &>);

    REQUIRE(MultAddable<bar &, const bar &, const bar &>);
    REQUIRE(!MultAddable<const bar &, const bar &, const bar &>);
    REQUIRE(!MultAddable<bar &&, const bar &, const bar &>);

    REQUIRE(!MultAddable<nobar &, const nobar &, const nobar &>);
    REQUIRE(!MultAddable<const nobar &, const nobar &, const nobar &>);
    REQUIRE(!MultAddable<nobar &&, const nobar &, const nobar &>);
#endif
}
