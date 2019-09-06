// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
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

#include <piranha/byte_size.hpp>
#include <piranha/config.hpp>
#include <piranha/type_traits.hpp>

#include "catch.hpp"

// Verify constexpr capability.
constexpr auto s_int = piranha::byte_size(0);

template <std::size_t>
struct foo_b {
};

[[maybe_unused]] static foo_b<piranha::byte_size(1)> fb;

TEST_CASE("byte_size_arith")
{
    REQUIRE(!piranha::is_size_measurable_v<void>);
    REQUIRE(piranha::is_size_measurable_v<float>);
    REQUIRE(piranha::is_size_measurable_v<int>);
    REQUIRE(piranha::is_size_measurable_v<double &&>);
    REQUIRE(piranha::is_size_measurable_v<const long double>);
    REQUIRE(piranha::is_size_measurable_v<short &>);
    REQUIRE(piranha::is_size_measurable_v<const char &>);

#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::is_size_measurable_v<__int128_t>);
    REQUIRE(piranha::is_size_measurable_v<__uint128_t>);
    REQUIRE(piranha::is_size_measurable_v<__int128_t &&>);
    REQUIRE(piranha::is_size_measurable_v<const __uint128_t>);
    REQUIRE(piranha::is_size_measurable_v<const __uint128_t &>);
    REQUIRE(piranha::is_size_measurable_v<__int128_t &>);
#endif

#if defined(MPPP_WITH_QUADMATH)
    REQUIRE(piranha::is_size_measurable_v<mppp::real128>);
    REQUIRE(piranha::is_size_measurable_v<mppp::real128 &>);
    REQUIRE(piranha::is_size_measurable_v<mppp::real128 &&>);
    REQUIRE(piranha::is_size_measurable_v<const mppp::real128 &>);
#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!piranha::SizeMeasurable<void>);
    REQUIRE(piranha::SizeMeasurable<float>);
    REQUIRE(piranha::SizeMeasurable<int>);
    REQUIRE(piranha::SizeMeasurable<double &&>);
    REQUIRE(piranha::SizeMeasurable<const long double>);
    REQUIRE(piranha::SizeMeasurable<short &>);
    REQUIRE(piranha::SizeMeasurable<const char &>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::SizeMeasurable<__int128_t>);
    REQUIRE(piranha::SizeMeasurable<__uint128_t>);
    REQUIRE(piranha::SizeMeasurable<__int128_t &&>);
    REQUIRE(piranha::SizeMeasurable<const __uint128_t>);
    REQUIRE(piranha::SizeMeasurable<const __uint128_t &>);
    REQUIRE(piranha::SizeMeasurable<__int128_t &>);
#endif
#if defined(MPPP_WITH_QUADMATH)
    REQUIRE(piranha::SizeMeasurable<mppp::real128>);
    REQUIRE(piranha::SizeMeasurable<mppp::real128 &>);
    REQUIRE(piranha::SizeMeasurable<mppp::real128 &&>);
    REQUIRE(piranha::SizeMeasurable<const mppp::real128 &>);
#endif
#endif

    REQUIRE(s_int == sizeof(int));
    REQUIRE(piranha::byte_size(0) == sizeof(int));
    REQUIRE(piranha::byte_size(0u) == sizeof(unsigned));
    REQUIRE(piranha::byte_size(short(0)) == sizeof(short));
    REQUIRE(piranha::byte_size(0.) == sizeof(double));
    REQUIRE(piranha::byte_size(0.f) == sizeof(float));
    REQUIRE(piranha::byte_size(42ll) == sizeof(long long));
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(piranha::byte_size(__int128_t(0)) == 16u);
    REQUIRE(piranha::byte_size(__uint128_t(0)) == 16u);
#endif
#if defined(MPPP_WITH_QUADMATH)
    REQUIRE(piranha::byte_size(mppp::real128(0)) == sizeof(mppp::real128));
#endif
}

TEST_CASE("byte_size_mp++_int")
{
    using int_t = mppp::integer<1>;

    REQUIRE(piranha::is_size_measurable_v<int_t>);
    REQUIRE(piranha::is_size_measurable_v<int_t &>);
    REQUIRE(piranha::is_size_measurable_v<const int_t &>);
    REQUIRE(piranha::is_size_measurable_v<int_t &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::SizeMeasurable<int_t>);
    REQUIRE(piranha::SizeMeasurable<int_t &>);
    REQUIRE(piranha::SizeMeasurable<const int_t &>);
    REQUIRE(piranha::SizeMeasurable<int_t &&>);
#endif

    REQUIRE(piranha::byte_size(int_t(0)) == sizeof(int_t));
    int_t n{42}, nc(n);
    nc.promote();
    REQUIRE(piranha::byte_size(nc) > piranha::byte_size(n));
    REQUIRE(piranha::byte_size(nc) == piranha::byte_size(n) + sizeof(::mp_limb_t));
}

TEST_CASE("byte_size_mp++_rat")
{
    using rat_t = mppp::rational<1>;

    REQUIRE(piranha::is_size_measurable_v<rat_t>);
    REQUIRE(piranha::is_size_measurable_v<rat_t &>);
    REQUIRE(piranha::is_size_measurable_v<const rat_t &>);
    REQUIRE(piranha::is_size_measurable_v<rat_t &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::SizeMeasurable<rat_t>);
    REQUIRE(piranha::SizeMeasurable<rat_t &>);
    REQUIRE(piranha::SizeMeasurable<const rat_t &>);
    REQUIRE(piranha::SizeMeasurable<rat_t &&>);
#endif

    REQUIRE(piranha::byte_size(rat_t{}) >= sizeof(rat_t));
    rat_t q{3, 4}, qc{q};
    qc._get_num().promote();
    qc._get_den().promote();
    REQUIRE(piranha::byte_size(qc) > piranha::byte_size(q));
}

#if defined(MPPP_WITH_MPFR)

TEST_CASE("byte_size_mp++_real")
{
    REQUIRE(piranha::is_size_measurable_v<mppp::real>);
    REQUIRE(piranha::is_size_measurable_v<mppp::real &>);
    REQUIRE(piranha::is_size_measurable_v<const mppp::real &>);
    REQUIRE(piranha::is_size_measurable_v<mppp::real &&>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::SizeMeasurable<mppp::real>);
    REQUIRE(piranha::SizeMeasurable<mppp::real &>);
    REQUIRE(piranha::SizeMeasurable<const mppp::real &>);
    REQUIRE(piranha::SizeMeasurable<mppp::real &&>);
#endif

    REQUIRE(piranha::byte_size(mppp::real{45}) > sizeof(mppp::real));
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

namespace piranha::customisation
{

template <typename T>
#if defined(PIRANHA_HAVE_CONCEPTS)
requires SameCvr<T, byte_size_1> inline constexpr auto byte_size<T>
#else
inline constexpr auto byte_size<T, std::enable_if_t<is_same_cvr_v<T, byte_size_1>>>
#endif
    = [](auto &&) constexpr noexcept
{
    return std::size_t(42);
};

} // namespace piranha::customisation

TEST_CASE("byte_size_custom")
{
    // Check type-traits/concepts.
    REQUIRE(piranha::is_size_measurable_v<byte_size_def>);
    REQUIRE(!piranha::is_size_measurable_v<no_byte_size_1>);
    REQUIRE(piranha::is_size_measurable_v<byte_size_0>);
    REQUIRE(piranha::is_size_measurable_v<byte_size_1>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(piranha::SizeMeasurable<byte_size_def>);
    REQUIRE(!piranha::SizeMeasurable<no_byte_size_1>);
    REQUIRE(piranha::SizeMeasurable<byte_size_0>);
    REQUIRE(piranha::SizeMeasurable<byte_size_1>);
#endif

    REQUIRE(piranha::byte_size(byte_size_def{}) == sizeof(byte_size_def));
    REQUIRE(piranha::byte_size(byte_size_0{}) == 41u);
    REQUIRE(piranha::byte_size(byte_size_1{}) == 42u);
}
