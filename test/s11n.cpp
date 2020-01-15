// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <sstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <obake/config.hpp>
#include <obake/s11n.hpp>

#include "catch.hpp"

TEST_CASE("s11n_test")
{
#if defined(OBAKE_HAVE_GCC_INT128)
    // Test serialisation for the 128-bit integral types.
    {
        static_assert(boost::serialization::tracking_level<__uint128_t>::value == boost::serialization::track_never);
        static_assert(boost::serialization::tracking_level<__int128_t>::value == boost::serialization::track_never);

        std::stringstream ss;
        __uint128_t out;

        {
            boost::archive::binary_oarchive oarchive(ss);
            oarchive << __uint128_t(0);
        }
        {
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> out;
        }
        REQUIRE(out == 0u);
        ss.str("");

        {
            boost::archive::binary_oarchive oarchive(ss);
            oarchive << __uint128_t(42);
        }
        {
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> out;
        }
        REQUIRE(out == 42u);
        ss.str("");

        {
            boost::archive::binary_oarchive oarchive(ss);
            oarchive << ((__uint128_t(42) << 64) + 42u);
        }
        {
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> out;
        }
        REQUIRE(out == ((__uint128_t(42) << 64) + 42u));
        ss.str("");
    }

    {
        std::stringstream ss;
        __int128_t out;

        {
            boost::archive::binary_oarchive oarchive(ss);
            oarchive << __int128_t(0);
        }
        {
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> out;
        }
        REQUIRE(out == 0);
        ss.str("");

        {
            boost::archive::binary_oarchive oarchive(ss);
            oarchive << __int128_t(42);
        }
        {
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> out;
        }
        REQUIRE(out == 42);
        ss.str("");

        {
            boost::archive::binary_oarchive oarchive(ss);
            oarchive << ((__int128_t(42) << 64) + 42);
        }
        {
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> out;
        }
        REQUIRE(out == ((__int128_t(42) << 64) + 42));
        ss.str("");

        {
            boost::archive::binary_oarchive oarchive(ss);
            oarchive << -__int128_t(42);
        }
        {
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> out;
        }
        REQUIRE(out == -42);
        ss.str("");

        {
            boost::archive::binary_oarchive oarchive(ss);
            oarchive << -((__int128_t(42) << 64) + 42);
        }
        {
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> out;
        }
        REQUIRE(out == -((__int128_t(42) << 64) + 42));
        ss.str("");
    }
#endif
}
