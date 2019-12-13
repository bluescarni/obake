// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <obake/config.hpp>

#include <initializer_list>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>

#if defined(OBAKE_HAVE_STRING_VIEW)
#include <string_view>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/variant/get.hpp>

#include <mp++/rational.hpp>

#include <obake/math/degree.hpp>
#include <obake/math/p_degree.hpp>
#include <obake/math/pow.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/power_series/truncated_power_series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

struct foo {
};

TEST_CASE("basic_tests")
{
    using tps_t = truncated_power_series<packed_monomial<int>, mppp::rational<1>>;
    using poly_t = tps_t::poly_t;
    using tps_t_d = truncated_power_series<packed_monomial<int>, double>;
    using trunc_t = tps_t::trunc_t;

    // Concepts.
    REQUIRE(power_series::is_tps_cf_v<int>);
    REQUIRE(!power_series::is_tps_cf_v<void>);
    // NOTE: not a tps cf because it has a degree.
    REQUIRE(!power_series::is_tps_cf_v<poly_t>);
    REQUIRE(power_series::is_tps_key_v<packed_monomial<int>>);
    REQUIRE(!power_series::is_tps_key_v<void>);
    REQUIRE(!power_series::is_cvr_truncated_power_series_v<void>);
    REQUIRE(!power_series::is_cvr_truncated_power_series_v<poly_t>);
    REQUIRE(power_series::is_cvr_truncated_power_series_v<tps_t>);
    REQUIRE(power_series::is_cvr_truncated_power_series_v<tps_t &>);
    REQUIRE(power_series::is_cvr_truncated_power_series_v<const tps_t &>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(power_series::TPSCf<int>);
    REQUIRE(!power_series::TPSCf<void>);
    REQUIRE(!power_series::TPSCf<poly_t>);
    REQUIRE(power_series::TPSKey<packed_monomial<int>>);
    REQUIRE(!power_series::TPSKey<void>);
    REQUIRE(!power_series::CvrTruncatedPowerSeries<void>);
    REQUIRE(!power_series::CvrTruncatedPowerSeries<poly_t>);
    REQUIRE(power_series::CvrTruncatedPowerSeries<tps_t>);
    REQUIRE(power_series::CvrTruncatedPowerSeries<tps_t &>);
    REQUIRE(power_series::CvrTruncatedPowerSeries<const tps_t &>);
#endif

    // Def ctor.
    tps_t t00;
    REQUIRE(t00._poly().empty());
    REQUIRE(t00._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t00._trunc().which() == 0);

    // Generic ctor from scalar.
    tps_t t01{42};
    REQUIRE(t01._poly() == 42);
    REQUIRE(t01._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t01._trunc().which() == 0);
    REQUIRE(!std::is_constructible_v<tps_t, void>);
    REQUIRE(!std::is_constructible_v<tps_t, foo>);

    // Generic ctor from other tps type.
    tps_t t02{tps_t_d{42.}};
    REQUIRE(t02._poly() == 42);
    REQUIRE(t02._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t02._trunc().which() == 0);

    // Generic ctor from other tps type.
    tps_t t03{tps_t_d{42., 4}};
    REQUIRE(t03._poly() == 42);
    REQUIRE(t03._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t03._trunc().which() == 1);
    REQUIRE(boost::get<int>(t03._trunc()) == 4);

    // Copy ctor.
    tps_t t03_copy{t03};
    REQUIRE(t03_copy._poly() == 42);
    REQUIRE(t03_copy._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t03_copy._trunc().which() == 1);
    REQUIRE(boost::get<int>(t03_copy._trunc()) == 4);

    // Move ctor.
    tps_t t03_move{std::move(t03)};
    REQUIRE(t03_move._poly() == 42);
    REQUIRE(t03_move._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t03_move._trunc().which() == 1);
    REQUIRE(boost::get<int>(t03_move._trunc()) == 4);

    // Generic ctor from other tps type, with partial trunc.
    tps_t t04{tps_t_d{42., 4, symbol_set{"x", "y"}}};
    REQUIRE(t04._poly() == 42);
    REQUIRE(t04._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t04._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t04._trunc())) == 4);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t04._trunc())) == symbol_set{"x", "y"});

    // Constructor from generic object + symbol set.
    tps_t t05{42, symbol_set{"x", "y"}};
    REQUIRE(t05._poly() == 42);
    REQUIRE(t05._poly().get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(t05._trunc().which() == 0);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<tps_t, void, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<tps_t, foo, const symbol_set &>);

    // Constructor from generic object + total degree truncation.
    tps_t t06{42, 4};
    REQUIRE(t06._poly() == 42);
    REQUIRE(t06._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t06._trunc().which() == 1);
    REQUIRE(boost::get<int>(t06._trunc()) == 4);
    tps_t t07{42, -1l};
    REQUIRE(t07._poly().empty());
    REQUIRE(t07._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t07._trunc().which() == 1);
    REQUIRE(boost::get<int>(t07._trunc()) == -1);
    tps_t t08{obake::pow(make_polynomials<poly_t>("x")[0], 2), 2u};
    REQUIRE(t08._poly() == obake::pow(make_polynomials<poly_t>("x")[0], 2));
    REQUIRE(t08._poly().get_symbol_set() == symbol_set{"x"});
    REQUIRE(t08._trunc().which() == 1);
    REQUIRE(boost::get<int>(t08._trunc()) == 2);
    tps_t t09{obake::pow(make_polynomials<poly_t>("x")[0], 2), 1ull};
    REQUIRE(t09._poly().empty());
    REQUIRE(t09._poly().get_symbol_set() == symbol_set{"x"});
    REQUIRE(t09._trunc().which() == 1);
    REQUIRE(boost::get<int>(t09._trunc()) == 1);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, int>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, void>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, foo>);
    REQUIRE(!std::is_constructible_v<tps_t, int, foo>);

    // Constructor from generic object + symbol set + total degree truncation.
    tps_t t10{42, symbol_set{"x", "y"}, 4};
    REQUIRE(t10._poly() == 42);
    REQUIRE(t10._poly().get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(t10._trunc().which() == 1);
    REQUIRE(boost::get<int>(t10._trunc()) == 4);
    tps_t t11{42, symbol_set{"x", "y"}, -1l};
    REQUIRE(t11._poly().empty());
    REQUIRE(t11._poly().get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(t11._trunc().which() == 1);
    REQUIRE(boost::get<int>(t11._trunc()) == -1);
    REQUIRE(!std::is_constructible_v<tps_t, const poly_t &, const symbol_set &, int>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, const symbol_set &, int>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, const symbol_set &, void>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, const symbol_set &, foo>);
    REQUIRE(!std::is_constructible_v<tps_t, int, const symbol_set &, foo>);

    // Constructor from generic object + partial degree truncation.
    tps_t t12{42, 4, symbol_set{"x", "y"}};
    REQUIRE(t12._poly() == 42);
    REQUIRE(t12._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t12._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t12._trunc())) == 4);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t12._trunc())) == symbol_set{"x", "y"});
    tps_t t13{42, -1l, symbol_set{"x", "y"}};
    REQUIRE(t13._poly().empty());
    REQUIRE(t13._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t13._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t13._trunc())) == -1);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t13._trunc())) == symbol_set{"x", "y"});
    tps_t t14{obake::pow(make_polynomials<poly_t>("x")[0], 2), 2u, symbol_set{"x", "y"}};
    REQUIRE(t14._poly() == obake::pow(make_polynomials<poly_t>("x")[0], 2));
    REQUIRE(t14._poly().get_symbol_set() == symbol_set{"x"});
    REQUIRE(t14._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t14._trunc())) == 2);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t14._trunc())) == symbol_set{"x", "y"});
    tps_t t15{obake::pow(make_polynomials<poly_t>("x")[0], 2), 1ull, symbol_set{"x", "y"}};
    REQUIRE(t15._poly().empty());
    REQUIRE(t15._poly().get_symbol_set() == symbol_set{"x"});
    REQUIRE(t15._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t15._trunc())) == 1);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t15._trunc())) == symbol_set{"x", "y"});
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, int, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, void, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, foo, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<tps_t, int, foo, const symbol_set &>);

    // Constructor from generic object + symbol set + partial degree truncation.
    tps_t t16{42, symbol_set{"x", "y"}, 4, symbol_set{"x"}};
    REQUIRE(t16._poly() == 42);
    REQUIRE(t16._poly().get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(t16._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t16._trunc())) == 4);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t16._trunc())) == symbol_set{"x"});
    tps_t t17{42, symbol_set{"x", "y"}, -1l, symbol_set{"x"}};
    REQUIRE(t17._poly().empty());
    REQUIRE(t17._poly().get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(t17._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t17._trunc())) == -1);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t17._trunc())) == symbol_set{"x"});
    REQUIRE(!std::is_constructible_v<tps_t, const poly_t &, const symbol_set &, int, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, const symbol_set &, int, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, const symbol_set &, void, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, const symbol_set &, foo, const symbol_set &>);
    REQUIRE(!std::is_constructible_v<tps_t, int, const symbol_set &, foo, const symbol_set &>);

    // Generic assignment operator.
    REQUIRE(!std::is_assignable_v<tps_t &, void>);
    t00 = 41;
    REQUIRE(t00._poly() == 41);
    REQUIRE(t00._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t00._trunc().which() == 0);

    t00 = make_polynomials<poly_t>("x")[0];
    REQUIRE(t00._poly() == make_polynomials<poly_t>("x")[0]);
    REQUIRE(t00._poly().get_symbol_set() == symbol_set{"x"});
    REQUIRE(t00._trunc().which() == 0);

    t00 = tps_t_d{42.};
    REQUIRE(t00._poly() == 42);
    REQUIRE(t00._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t00._trunc().which() == 0);

    t00 = tps_t_d{42., 4, symbol_set{"x", "y"}};
    REQUIRE(t00._poly() == 42);
    REQUIRE(t00._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t00._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t00._trunc())) == 4);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t00._trunc())) == symbol_set{"x", "y"});

    // Swapping.
    REQUIRE(std::is_nothrow_swappable_v<tps_t>);
    t01 = tps_t_d{41., 3, symbol_set{"x", "y", "z"}};
    using std::swap;
    swap(t01, t00);

    REQUIRE(t01._poly() == 42);
    REQUIRE(t01._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t01._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t01._trunc())) == 4);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t01._trunc())) == symbol_set{"x", "y"});
    REQUIRE(t00._poly() == 41);
    REQUIRE(t00._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t00._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t00._trunc())) == 3);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t00._trunc())) == symbol_set{"x", "y", "z"});

    // Streaming.
    std::ostringstream oss;
    oss << t01;
    auto str = oss.str();
    REQUIRE(boost::contains(str, "4, {'x', 'y'}"));
    oss.str("");
    oss << tps_t{42};
    str = oss.str();
    REQUIRE(boost::contains(str, "None"));
    oss.str("");
    oss << tps_t{42, 32};
    str = oss.str();
    REQUIRE(boost::contains(str, "32"));
    oss.str("");
    oss << make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, "x")[0];
    str = oss.str();
    REQUIRE(boost::contains(str, "None"));
    REQUIRE(boost::contains(str, "{'x', 'y', 'z'}"));
    REQUIRE(boost::contains(str, "Truncation"));
    REQUIRE(boost::contains(str, "Rank"));
    REQUIRE(boost::contains(str, "Symbol set"));

    // Constructor from generic object + trunc_t.
    tps_t t18{42, trunc_t{}};
    REQUIRE(t18._poly() == 42);
    REQUIRE(t18._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t18._trunc().which() == 0);
    tps_t t19{42, trunc_t{10}};
    REQUIRE(t19._poly() == 42);
    REQUIRE(t19._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t19._trunc().which() == 1);
    REQUIRE(boost::get<int>(t19._trunc()) == 10);
    tps_t t20{42, trunc_t{std::make_tuple(9, symbol_set{"x", "y"})}};
    REQUIRE(t20._poly() == 42);
    REQUIRE(t20._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t20._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t20._trunc())) == 9);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t20._trunc())) == symbol_set{"x", "y"});
    tps_t t21{42, trunc_t{-2}};
    REQUIRE(t21._poly().empty());
    REQUIRE(t21._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t21._trunc().which() == 1);
    REQUIRE(boost::get<int>(t21._trunc()) == -2);
    tps_t t22{42, trunc_t{std::make_tuple(-1, symbol_set{"x", "y"})}};
    REQUIRE(t22._poly().empty());
    REQUIRE(t22._poly().get_symbol_set() == symbol_set{});
    REQUIRE(t22._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t22._trunc())) == -1);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t22._trunc())) == symbol_set{"x", "y"});
    tps_t t23{obake::pow(make_polynomials<poly_t>("x")[0], 2), trunc_t{std::make_tuple(2u, symbol_set{"x", "y"})}};
    REQUIRE(t23._poly() == obake::pow(make_polynomials<poly_t>("x")[0], 2));
    REQUIRE(t23._poly().get_symbol_set() == symbol_set{"x"});
    REQUIRE(t23._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t23._trunc())) == 2);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t23._trunc())) == symbol_set{"x", "y"});

    // Constructor from generic object + symbol set + trunc_t.
    tps_t t24{42, symbol_set{"x", "y"}, trunc_t{std::make_tuple(4, symbol_set{"x"})}};
    REQUIRE(t24._poly() == 42);
    REQUIRE(t24._poly().get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(t24._trunc().which() == 2);
    REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(t24._trunc())) == 4);
    REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(t24._trunc())) == symbol_set{"x"});
    tps_t t25{42, symbol_set{"x", "y"}, trunc_t{-1}};
    REQUIRE(t25._poly().empty());
    REQUIRE(t25._poly().get_symbol_set() == symbol_set{"x", "y"});
    REQUIRE(t25._trunc().which() == 1);
    REQUIRE(boost::get<int>(t25._trunc()) == -1);
    REQUIRE(!std::is_constructible_v<tps_t, const poly_t &, const symbol_set &, const trunc_t &>);
    REQUIRE(!std::is_constructible_v<tps_t, const tps_t &, const symbol_set &, const trunc_t &>);
}

template <typename T, typename... Args>
using make_tps_t = decltype(make_truncated_power_series<T>(std::declval<Args>()...));

TEST_CASE("make_tps_test")
{
    using tps_t = truncated_power_series<packed_monomial<int>, mppp::rational<1>>;
    using trunc_t = tps_t::trunc_t;
    using poly_t = typename tps_t::poly_t;

    // Generators only.
    {
        REQUIRE(make_truncated_power_series<tps_t>().empty());

        auto [x, y, z] = make_truncated_power_series<tps_t>("x", "y", "z");

        REQUIRE(x._poly() == make_polynomials<poly_t>("x")[0]);
        REQUIRE(x._poly().get_symbol_set() == symbol_set{"x"});
        REQUIRE(x._trunc().which() == 0);

        REQUIRE(y._poly() == make_polynomials<poly_t>("y")[0]);
        REQUIRE(y._poly().get_symbol_set() == symbol_set{"y"});
        REQUIRE(y._trunc().which() == 0);

        REQUIRE(z._poly() == make_polynomials<poly_t>("z")[0]);
        REQUIRE(z._poly().get_symbol_set() == symbol_set{"z"});
        REQUIRE(z._trunc().which() == 0);
    }

    // Generators + symbol set.
    {
        REQUIRE(make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}).empty());

        auto [x, y, z] = make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, "x", "y", "z");

        REQUIRE(x._poly() == make_polynomials<poly_t>("x")[0]);
        REQUIRE(x._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(x._trunc().which() == 0);

        REQUIRE(y._poly() == make_polynomials<poly_t>("y")[0]);
        REQUIRE(y._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(y._trunc().which() == 0);

        REQUIRE(z._poly() == make_polynomials<poly_t>("z")[0]);
        REQUIRE(z._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(z._trunc().which() == 0);
    }

    // Generators + trunc_t.
    {
        REQUIRE(make_truncated_power_series<tps_t>(trunc_t{3}).empty());

        auto [x, y, z] = make_truncated_power_series<tps_t>(trunc_t{3}, "x", "y", "z");

        REQUIRE(x._poly() == make_polynomials<poly_t>("x")[0]);
        REQUIRE(x._poly().get_symbol_set() == symbol_set{"x"});
        REQUIRE(x._trunc().which() == 1);
        REQUIRE(boost::get<int>(x._trunc()) == 3);

        REQUIRE(y._poly() == make_polynomials<poly_t>("y")[0]);
        REQUIRE(y._poly().get_symbol_set() == symbol_set{"y"});
        REQUIRE(y._trunc().which() == 1);
        REQUIRE(boost::get<int>(y._trunc()) == 3);

        REQUIRE(z._poly() == make_polynomials<poly_t>("z")[0]);
        REQUIRE(z._poly().get_symbol_set() == symbol_set{"z"});
        REQUIRE(z._trunc().which() == 1);
        REQUIRE(boost::get<int>(z._trunc()) == 3);
    }

    // Generators + symbol set + trunc_t.
    {
        REQUIRE(make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, trunc_t{3}).empty());

#if defined(OBAKE_HAVE_STRING_VIEW)
        auto [x, y, z] = make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, trunc_t{3},
                                                            std::string_view{"x"}, "y", std::string_view{"z"});
#else
        auto [x, y, z] = make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, trunc_t{3}, "x", "y", "z");
#endif

        REQUIRE(x._poly() == make_polynomials<poly_t>("x")[0]);
        REQUIRE(x._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(x._trunc().which() == 1);
        REQUIRE(boost::get<int>(x._trunc()) == 3);

        REQUIRE(y._poly() == make_polynomials<poly_t>("y")[0]);
        REQUIRE(y._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(y._trunc().which() == 1);
        REQUIRE(boost::get<int>(y._trunc()) == 3);

        REQUIRE(z._poly() == make_polynomials<poly_t>("z")[0]);
        REQUIRE(z._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(z._trunc().which() == 1);
        REQUIRE(boost::get<int>(z._trunc()) == 3);
    }

    // Generators + total degree truncation.
    {
        REQUIRE(make_truncated_power_series<tps_t>(3).empty());

        auto [x, y, z] = make_truncated_power_series<tps_t>(3, "x", "y", "z");

        REQUIRE(x._poly() == make_polynomials<poly_t>("x")[0]);
        REQUIRE(x._poly().get_symbol_set() == symbol_set{"x"});
        REQUIRE(x._trunc().which() == 1);
        REQUIRE(boost::get<int>(x._trunc()) == 3);

        REQUIRE(y._poly() == make_polynomials<poly_t>("y")[0]);
        REQUIRE(y._poly().get_symbol_set() == symbol_set{"y"});
        REQUIRE(y._trunc().which() == 1);
        REQUIRE(boost::get<int>(y._trunc()) == 3);

        REQUIRE(z._poly() == make_polynomials<poly_t>("z")[0]);
        REQUIRE(z._poly().get_symbol_set() == symbol_set{"z"});
        REQUIRE(z._trunc().which() == 1);
        REQUIRE(boost::get<int>(z._trunc()) == 3);
    }

    // Generators + symbol set + total degree truncation.
    {
        REQUIRE(make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, 3).empty());

#if defined(OBAKE_HAVE_STRING_VIEW)
        auto [x, y, z] = make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, 3, std::string_view{"x"}, "y",
                                                            std::string_view{"z"});
#else
        auto [x, y, z] = make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, 3, "x", "y", "z");
#endif

        REQUIRE(x._poly() == make_polynomials<poly_t>("x")[0]);
        REQUIRE(x._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(x._trunc().which() == 1);
        REQUIRE(boost::get<int>(x._trunc()) == 3);

        REQUIRE(y._poly() == make_polynomials<poly_t>("y")[0]);
        REQUIRE(y._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(y._trunc().which() == 1);
        REQUIRE(boost::get<int>(y._trunc()) == 3);

        REQUIRE(z._poly() == make_polynomials<poly_t>("z")[0]);
        REQUIRE(z._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(z._trunc().which() == 1);
        REQUIRE(boost::get<int>(z._trunc()) == 3);
    }

    // Generators + partial degree truncation.
    {
        REQUIRE(make_truncated_power_series<tps_t>(3, symbol_set{"x", "y"}).empty());

#if defined(OBAKE_HAVE_STRING_VIEW)
        auto [x, y, z] = make_truncated_power_series<tps_t>(3, symbol_set{"x", "y"}, "x", "y", std::string_view{"z"});
#else
        auto [x, y, z] = make_truncated_power_series<tps_t>(3, symbol_set{"x", "y"}, "x", "y", "z");
#endif

        REQUIRE(x._poly() == make_polynomials<poly_t>("x")[0]);
        REQUIRE(x._poly().get_symbol_set() == symbol_set{"x"});
        REQUIRE(x._trunc().which() == 2);
        REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(x._trunc())) == 3);
        REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(x._trunc())) == symbol_set{"x", "y"});

        REQUIRE(y._poly() == make_polynomials<poly_t>("y")[0]);
        REQUIRE(y._poly().get_symbol_set() == symbol_set{"y"});
        REQUIRE(y._trunc().which() == 2);
        REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(y._trunc())) == 3);
        REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(y._trunc())) == symbol_set{"x", "y"});

        REQUIRE(z._poly() == make_polynomials<poly_t>("z")[0]);
        REQUIRE(z._poly().get_symbol_set() == symbol_set{"z"});
        REQUIRE(z._trunc().which() == 2);
        REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(z._trunc())) == 3);
        REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(z._trunc())) == symbol_set{"x", "y"});
    }

    // Generators + symbol set + partial degree truncation.
    {
        REQUIRE(make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, 3, symbol_set{"x", "y"}).empty());

#if defined(OBAKE_HAVE_STRING_VIEW)
        auto [x, y, z] = make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, 3, symbol_set{"x", "y"}, "x",
                                                            std::string_view{"y"}, "z");
#else
        auto [x, y, z]
            = make_truncated_power_series<tps_t>(symbol_set{"x", "y", "z"}, 3, symbol_set{"x", "y"}, "x", "y", "z");
#endif

        REQUIRE(x._poly() == make_polynomials<poly_t>("x")[0]);
        REQUIRE(x._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(x._trunc().which() == 2);
        REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(x._trunc())) == 3);
        REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(x._trunc())) == symbol_set{"x", "y"});

        REQUIRE(y._poly() == make_polynomials<poly_t>("y")[0]);
        REQUIRE(y._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(y._trunc().which() == 2);
        REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(y._trunc())) == 3);
        REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(y._trunc())) == symbol_set{"x", "y"});

        REQUIRE(z._poly() == make_polynomials<poly_t>("z")[0]);
        REQUIRE(z._poly().get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(z._trunc().which() == 2);
        REQUIRE(std::get<0>(boost::get<std::tuple<int, symbol_set>>(z._trunc())) == 3);
        REQUIRE(std::get<1>(boost::get<std::tuple<int, symbol_set>>(z._trunc())) == symbol_set{"x", "y"});
    }

    // Check SFINAEing.
    REQUIRE(!is_detected_v<make_tps_t, void>);
    REQUIRE(!is_detected_v<make_tps_t, int>);
    REQUIRE(!is_detected_v<make_tps_t, void, trunc_t>);
    REQUIRE(!is_detected_v<make_tps_t, void, trunc_t, symbol_set>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, trunc_t, int>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, trunc_t, symbol_set>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, int, trunc_t>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, trunc_t, int>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, trunc_t, symbol_set>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, void>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, void, void>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, void, void, void>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, void, int>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, int, void, void>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, int, void, int>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, void, int, int>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, int, int>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, int, const symbol_set &, const symbol_set &>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, int, const symbol_set &, int>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, int, const symbol_set &, std::string, std::string, int>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, int, const symbol_set &, int, std::string, std::string>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, int, int, const symbol_set &, std::string, std::string>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, int, const symbol_set &, std::string, std::string, void, std::string>);
    REQUIRE(!is_detected_v<make_tps_t, void, int, const symbol_set &, std::string, std::string>);
    REQUIRE(!is_detected_v<make_tps_t, tps_t, std::string, const symbol_set &, std::string, std::string>);
}

TEST_CASE("degree_tests")
{
    using tps_t = truncated_power_series<packed_monomial<int>, mppp::rational<1>>;
    using poly_t = tps_t::poly_t;

    REQUIRE(is_with_degree_v<const tps_t &>);
    REQUIRE(is_with_p_degree_v<const tps_t &>);

    REQUIRE(degree(tps_t{}) == 0);
    REQUIRE(p_degree(tps_t{}, symbol_set{"x", "y"}) == 0);

    REQUIRE(degree(make_truncated_power_series<tps_t>("x")[0]) == 1);
    REQUIRE(degree(obake::pow(make_polynomials<poly_t>("x")[0], -10)) == -10);
    REQUIRE(p_degree(make_truncated_power_series<tps_t>("x")[0], symbol_set{"x", "y", "z"}) == 1);
    REQUIRE(p_degree(make_truncated_power_series<tps_t>("x")[0], symbol_set{"y", "z"}) == 0);
}
