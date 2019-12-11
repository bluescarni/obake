// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>

#include <mp++/rational.hpp>

#include <obake/polynomials/packed_monomial.hpp>
#include <obake/power_series/truncated_power_series.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("binary_op_tmp_tests")
{
    using tps_t = truncated_power_series<packed_monomial<int>, mppp::rational<1>>;
    using tps_t_d = truncated_power_series<packed_monomial<int>, double>;

    REQUIRE(!power_series::detail::tps_binary_op_algo<detail::add_t, void, void>);
    REQUIRE(!power_series::detail::tps_binary_op_algo<detail::add_t, void, tps_t>);
    REQUIRE(!power_series::detail::tps_binary_op_algo<detail::add_t, tps_t, void>);
    REQUIRE(!power_series::detail::tps_binary_op_algo<detail::add_t, int, int>);

    REQUIRE(power_series::detail::tps_binary_op_algo<detail::add_t, tps_t, tps_t>);
    REQUIRE(std::is_same_v<power_series::detail::tps_binary_op_ret_t<detail::add_t, tps_t, tps_t>, tps_t>);

    REQUIRE(power_series::detail::tps_binary_op_algo<detail::add_t, int, tps_t>);
    REQUIRE(std::is_same_v<power_series::detail::tps_binary_op_ret_t<detail::add_t, int, tps_t>, tps_t>);

    REQUIRE(power_series::detail::tps_binary_op_algo<detail::add_t, tps_t, int>);
    REQUIRE(std::is_same_v<power_series::detail::tps_binary_op_ret_t<detail::add_t, tps_t, int>, tps_t>);

    REQUIRE(power_series::detail::tps_binary_op_algo<detail::add_t, double, tps_t>);
    REQUIRE(std::is_same_v<power_series::detail::tps_binary_op_ret_t<detail::add_t, double, tps_t>, tps_t_d>);

    REQUIRE(power_series::detail::tps_binary_op_algo<detail::add_t, tps_t, double>);
    REQUIRE(std::is_same_v<power_series::detail::tps_binary_op_ret_t<detail::add_t, tps_t, double>, tps_t_d>);

    REQUIRE(power_series::detail::tps_binary_op_algo<detail::add_t, mppp::rational<1>, tps_t_d>);
    REQUIRE(
        std::is_same_v<power_series::detail::tps_binary_op_ret_t<detail::add_t, mppp::rational<1>, tps_t_d>, tps_t_d>);

    REQUIRE(power_series::detail::tps_binary_op_algo<detail::add_t, tps_t_d, mppp::rational<1>>);
    REQUIRE(
        std::is_same_v<power_series::detail::tps_binary_op_ret_t<detail::add_t, tps_t_d, mppp::rational<1>>, tps_t_d>);
}
