// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdint>
#include <initializer_list>
#include <list>
#include <random>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <vector>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <obake/config.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/key/key_tex_stream_insert.hpp>
#include <obake/kpack.hpp>
#include <obake/polynomials/monomial_range_overflow_check.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/s11n.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

static std::mt19937 rng;

using namespace obake;

using int_types = std::tuple<std::int32_t, std::uint32_t
#if defined(OBAKE_PACKABLE_INT64)
                             ,
                             std::int64_t, std::uint64_t
#endif
                             >;

TEST_CASE("key_stream_insert_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = ::std::remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(is_tex_stream_insertable_key_v<pm_t>);
        REQUIRE(is_tex_stream_insertable_key_v<pm_t &>);
        REQUIRE(is_tex_stream_insertable_key_v<const pm_t &>);
        REQUIRE(is_tex_stream_insertable_key_v<const pm_t>);

        std::ostringstream oss;

        key_tex_stream_insert(oss, pm_t{}, symbol_set{});
        REQUIRE(oss.str() == "1");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{1}, symbol_set{"x"});
        REQUIRE(oss.str() == "{x}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{1, 2}, symbol_set{"x", "y"});
        REQUIRE(oss.str() == "{x}{y}^{2}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{0, 2}, symbol_set{"x", "y"});
        REQUIRE(oss.str() == "{y}^{2}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{1, 0}, symbol_set{"x", "y"});
        REQUIRE(oss.str() == "{x}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{2, 0}, symbol_set{"x", "y"});
        REQUIRE(oss.str() == "{x}^{2}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{2, 0, 1}, symbol_set{"x", "y", "z"});
        REQUIRE(oss.str() == "{x}^{2}{z}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{1, 2, 3}, symbol_set{"x", "y", "z"});
        REQUIRE(oss.str() == "{x}{y}^{2}{z}^{3}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{0, 0, 1}, symbol_set{"x", "y", "z"});
        REQUIRE(oss.str() == "{z}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{0, 0, 4}, symbol_set{"x", "y", "z"});
        REQUIRE(oss.str() == "{z}^{4}");
        oss.str("");

        key_tex_stream_insert(oss, pm_t{0, 0, 0}, symbol_set{"x", "y", "z"});
        REQUIRE(oss.str() == "1");
        oss.str("");

        if constexpr (is_signed_v<int_t>) {
            key_tex_stream_insert(oss, pm_t{-1}, symbol_set{"x"});
            REQUIRE(oss.str() == "\\frac{1}{{x}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{-1, -2}, symbol_set{"x", "y"});
            REQUIRE(oss.str() == "\\frac{1}{{x}{y}^{2}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{0, -2}, symbol_set{"x", "y"});
            REQUIRE(oss.str() == "\\frac{1}{{y}^{2}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{-1, 0}, symbol_set{"x", "y"});
            REQUIRE(oss.str() == "\\frac{1}{{x}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{-1, -2, -3}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{1}{{x}{y}^{2}{z}^{3}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{1, -2, -3}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{{x}}{{y}^{2}{z}^{3}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{2, -2, -3}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{{x}^{2}}{{y}^{2}{z}^{3}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{2, -2, 3}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{{x}^{2}{z}^{3}}{{y}^{2}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{-2, -2, 3}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{{z}^{3}}{{x}^{2}{y}^{2}}");
            oss.str("");

            key_tex_stream_insert(oss, pm_t{-2, 0, 0}, symbol_set{"x", "y", "z"});
            REQUIRE(oss.str() == "\\frac{1}{{x}^{2}}");
            oss.str("");
        }
    });
}

TEST_CASE("s11n_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = ::std::remove_cvref_t<decltype(n)>;
        using pm_t = packed_monomial<int_t>;

        REQUIRE(boost::serialization::tracking_level<pm_t>::value == boost::serialization::track_never);

        std::stringstream ss;
        pm_t tmp;

        {
            boost::archive::binary_oarchive oarchive(ss);
            oarchive << pm_t{1, 2, 3};
        }
        {
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> tmp;
        }
        REQUIRE(tmp == pm_t{1, 2, 3});
        ss.str("");

        {
            boost::archive::binary_oarchive oarchive(ss);
            oarchive << pm_t{};
        }
        {
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> tmp;
        }
        REQUIRE(tmp == pm_t{});
        ss.str("");

        if constexpr (is_signed_v<int_t>) {
            {
                boost::archive::binary_oarchive oarchive(ss);
                oarchive << pm_t{-1, 2, -3};
            }
            {
                boost::archive::binary_iarchive iarchive(ss);
                iarchive >> tmp;
            }
            REQUIRE(tmp == pm_t{-1, 2, -3});
            ss.str("");
        }
    });
}

// A test for exercising the multi-threaded monomial
// overflow check.
TEST_CASE("mt_overflow_check_test")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = ::std::remove_cvref_t<decltype(n)>;

        using pm_t = packed_monomial<int_t>;

        for (auto vs : {3u, 4u, 5u, 6u}) {
            symbol_set ss;
            for (auto j = 0u; j < vs; ++j) {
                ss.insert(ss.end(), "x_" + detail::to_string(j));
            }

            // Randomly generate a bunch of monomials with
            // exponents within the limits for the given vector size.
            std::vector<pm_t> v1;
            std::list<pm_t> l1;
            std::uniform_int_distribution<int_t> idist;
            std::vector<int_t> tmp(vs);
            for (auto i = 0u; i < 6000u; ++i) {
                for (auto j = 0u; j < vs; ++j) {
                    const auto lims = detail::kpack_get_lims<int_t>(vs);
                    tmp[j] = idist(rng,
                                   typename std::uniform_int_distribution<int_t>::param_type{lims.first, lims.second});
                    v1.emplace_back(tmp);
                    l1.emplace_back(tmp);
                }
            }
            // Create a range containing a single
            // unitary monomial. This will never
            // overflow when multiplied by v1/l1.
            std::vector<pm_t> v2(1, pm_t{ss});

            REQUIRE(monomial_range_overflow_check(v1, v2, ss));
            REQUIRE(monomial_range_overflow_check(v2, v1, ss));
            REQUIRE(monomial_range_overflow_check(l1, v2, ss));
            REQUIRE(monomial_range_overflow_check(v2, l1, ss));

            // Add monomials with maximal exponents.
            for (auto j = 0u; j < vs; ++j) {
                const auto lims = detail::kpack_get_lims<int_t>(vs);
                tmp[j] = lims.second;
            }
            v2[0] = pm_t(tmp);
            for (auto j = 0u; j < vs; ++j) {
                const auto lims = detail::kpack_get_lims<int_t>(vs);
                tmp[j] = lims.second;
            }
            v1.emplace_back(tmp);
            l1.emplace_back(tmp);

            REQUIRE(!monomial_range_overflow_check(v1, v2, ss));
            REQUIRE(!monomial_range_overflow_check(l1, v2, ss));
            REQUIRE(!monomial_range_overflow_check(v2, v1, ss));
            REQUIRE(!monomial_range_overflow_check(v2, l1, ss));
        }
    });
}
