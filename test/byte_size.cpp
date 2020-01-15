// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstddef>
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

#include <obake/byte_size.hpp>
#include <obake/config.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

// Verify constexpr capability.
constexpr auto s_int = obake::byte_size(0);

template <std::size_t>
struct foo_b {
};

[[maybe_unused]] static foo_b<obake::byte_size(1)> fb;

TEST_CASE("byte_size_arith")
{
    REQUIRE(!obake::is_size_measurable_v<void>);
    REQUIRE(obake::is_size_measurable_v<float>);
    REQUIRE(obake::is_size_measurable_v<int>);
    REQUIRE(obake::is_size_measurable_v<double &&>);
    REQUIRE(obake::is_size_measurable_v<const long double>);
    REQUIRE(obake::is_size_measurable_v<short &>);
    REQUIRE(obake::is_size_measurable_v<const char &>);

#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::is_size_measurable_v<__int128_t>);
    REQUIRE(obake::is_size_measurable_v<__uint128_t>);
    REQUIRE(obake::is_size_measurable_v<__int128_t &&>);
    REQUIRE(obake::is_size_measurable_v<const __uint128_t>);
    REQUIRE(obake::is_size_measurable_v<const __uint128_t &>);
    REQUIRE(obake::is_size_measurable_v<__int128_t &>);
#endif

#if defined(MPPP_WITH_QUADMATH)
    REQUIRE(obake::is_size_measurable_v<mppp::real128>);
    REQUIRE(obake::is_size_measurable_v<mppp::real128 &>);
    REQUIRE(obake::is_size_measurable_v<mppp::real128 &&>);
    REQUIRE(obake::is_size_measurable_v<const mppp::real128 &>);
#endif

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!obake::SizeMeasurable<void>);
    REQUIRE(obake::SizeMeasurable<float>);
    REQUIRE(obake::SizeMeasurable<int>);
    REQUIRE(obake::SizeMeasurable<double &&>);
    REQUIRE(obake::SizeMeasurable<const long double>);
    REQUIRE(obake::SizeMeasurable<short &>);
    REQUIRE(obake::SizeMeasurable<const char &>);
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::SizeMeasurable<__int128_t>);
    REQUIRE(obake::SizeMeasurable<__uint128_t>);
    REQUIRE(obake::SizeMeasurable<__int128_t &&>);
    REQUIRE(obake::SizeMeasurable<const __uint128_t>);
    REQUIRE(obake::SizeMeasurable<const __uint128_t &>);
    REQUIRE(obake::SizeMeasurable<__int128_t &>);
#endif
#if defined(MPPP_WITH_QUADMATH)
    REQUIRE(obake::SizeMeasurable<mppp::real128>);
    REQUIRE(obake::SizeMeasurable<mppp::real128 &>);
    REQUIRE(obake::SizeMeasurable<mppp::real128 &&>);
    REQUIRE(obake::SizeMeasurable<const mppp::real128 &>);
#endif
#endif

    REQUIRE(s_int == sizeof(int));
    REQUIRE(obake::byte_size(0) == sizeof(int));
    REQUIRE(obake::byte_size(0u) == sizeof(unsigned));
    REQUIRE(obake::byte_size(short(0)) == sizeof(short));
    REQUIRE(obake::byte_size(0.) == sizeof(double));
    REQUIRE(obake::byte_size(0.f) == sizeof(float));
    REQUIRE(obake::byte_size(42ll) == sizeof(long long));
#if defined(OBAKE_HAVE_GCC_INT128)
    REQUIRE(obake::byte_size(__int128_t(0)) == 16u);
    REQUIRE(obake::byte_size(__uint128_t(0)) == 16u);
#endif
#if defined(MPPP_WITH_QUADMATH)
    REQUIRE(obake::byte_size(mppp::real128(0)) == sizeof(mppp::real128));
#endif
}

TEST_CASE("byte_size_mp++_int")
{
    using int_t = mppp::integer<1>;

    REQUIRE(obake::is_size_measurable_v<int_t>);
    REQUIRE(obake::is_size_measurable_v<int_t &>);
    REQUIRE(obake::is_size_measurable_v<const int_t &>);
    REQUIRE(obake::is_size_measurable_v<int_t &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::SizeMeasurable<int_t>);
    REQUIRE(obake::SizeMeasurable<int_t &>);
    REQUIRE(obake::SizeMeasurable<const int_t &>);
    REQUIRE(obake::SizeMeasurable<int_t &&>);
#endif

    REQUIRE(obake::byte_size(int_t(0)) == sizeof(int_t));
    int_t n{42}, nc(n);
    nc.promote();
    REQUIRE(obake::byte_size(nc) > obake::byte_size(n));
    REQUIRE(obake::byte_size(nc) == obake::byte_size(n) + sizeof(::mp_limb_t));
}

TEST_CASE("byte_size_mp++_rat")
{
    using rat_t = mppp::rational<1>;

    REQUIRE(obake::is_size_measurable_v<rat_t>);
    REQUIRE(obake::is_size_measurable_v<rat_t &>);
    REQUIRE(obake::is_size_measurable_v<const rat_t &>);
    REQUIRE(obake::is_size_measurable_v<rat_t &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::SizeMeasurable<rat_t>);
    REQUIRE(obake::SizeMeasurable<rat_t &>);
    REQUIRE(obake::SizeMeasurable<const rat_t &>);
    REQUIRE(obake::SizeMeasurable<rat_t &&>);
#endif

    REQUIRE(obake::byte_size(rat_t{}) >= sizeof(rat_t));
    rat_t q{3, 4}, qc{q};
    qc._get_num().promote();
    qc._get_den().promote();
    REQUIRE(obake::byte_size(qc) > obake::byte_size(q));
}

#if defined(MPPP_WITH_MPFR)

TEST_CASE("byte_size_mp++_real")
{
    REQUIRE(obake::is_size_measurable_v<mppp::real>);
    REQUIRE(obake::is_size_measurable_v<mppp::real &>);
    REQUIRE(obake::is_size_measurable_v<const mppp::real &>);
    REQUIRE(obake::is_size_measurable_v<mppp::real &&>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::SizeMeasurable<mppp::real>);
    REQUIRE(obake::SizeMeasurable<mppp::real &>);
    REQUIRE(obake::SizeMeasurable<const mppp::real &>);
    REQUIRE(obake::SizeMeasurable<mppp::real &&>);
#endif

    REQUIRE(obake::byte_size(mppp::real{45}) > sizeof(mppp::real));
}

#endif

struct byte_size_def {
};

// Wrong return type in the ADL implementation.
struct no_byte_size_1 {
};

void byte_size(const no_byte_size_1 &);

// OK ADL implementation.
struct byte_size_0 {
};

std::size_t byte_size(const byte_size_0 &)
{
    return 41;
}

// External customisation point.
struct byte_size_1 {
};

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, byte_size_1> inline constexpr auto byte_size<T>
#else
inline constexpr auto byte_size<T, std::enable_if_t<is_same_cvr_v<T, byte_size_1>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return std::size_t(42);
};

} // namespace obake::customisation

TEST_CASE("byte_size_custom")
{
    // Check type-traits/concepts.
    REQUIRE(obake::is_size_measurable_v<byte_size_def>);
    REQUIRE(!obake::is_size_measurable_v<no_byte_size_1>);
    REQUIRE(obake::is_size_measurable_v<byte_size_0>);
    REQUIRE(obake::is_size_measurable_v<byte_size_1>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(obake::SizeMeasurable<byte_size_def>);
    REQUIRE(!obake::SizeMeasurable<no_byte_size_1>);
    REQUIRE(obake::SizeMeasurable<byte_size_0>);
    REQUIRE(obake::SizeMeasurable<byte_size_1>);
#endif

    REQUIRE(obake::byte_size(byte_size_def{}) == sizeof(byte_size_def));
    REQUIRE(obake::byte_size(byte_size_0{}) == 41u);
    REQUIRE(obake::byte_size(byte_size_1{}) == 42u);
}
