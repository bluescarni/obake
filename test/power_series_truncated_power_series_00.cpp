// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <obake/config.hpp>

#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <utility>

#if defined(OBAKE_HAVE_STRING_VIEW)
#include <string_view>
#endif

#include <boost/variant/get.hpp>

#include <mp++/rational.hpp>

#include <obake/math/degree.hpp>
#include <obake/math/p_degree.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/power_series/truncated_power_series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using namespace obake;

struct foo {
};

TEST_CASE("basic_test")
{
    using tps_t = truncated_power_series<packed_monomial<int>, mppp::rational<1>>;

    tps_t t00;
    REQUIRE(t00._poly().empty());
    REQUIRE(degree(t00) == 0);
    REQUIRE(p_degree(t00, symbol_set{}) == 0);

    tps_t{45};
    tps_t{std::string("423423")};

    REQUIRE(!std::is_constructible_v<tps_t, foo>);

    std::cout << tps_t{45} << '\n';
    std::cout << tps_t{45, 3} << '\n';
    std::cout << tps_t{45, 3u} << '\n';
    std::cout << tps_t{45, 3u, symbol_set{"x", "y", "z"}} << '\n';

    tps_t a{45};
    a = -42;
    std::cout << a << '\n';

    std::cout << tps_t{45, -1} << '\n';

    REQUIRE(std::is_nothrow_swappable_v<tps_t>);

    std::cout << tps_t{45, symbol_set{"x", "y", "z"}} << '\n';
    std::cout << tps_t{45, symbol_set{"x", "y", "z"}, 2} << '\n';
    std::cout << tps_t{45, symbol_set{"x", "y", "z"}, 2l} << '\n';
    std::cout << tps_t{45, symbol_set{"x", "y", "z"}, 2, symbol_set{"x"}} << '\n';
    std::cout << tps_t{45, symbol_set{"x", "y", "z"}, 2l, symbol_set{"x"}} << '\n';
}

template <typename T, typename... Args>
using make_tps_t = decltype(make_truncated_power_series<T>(std::declval<Args>()...));

TEST_CASE("make_tps_test")
{
    using tps_t = truncated_power_series<packed_monomial<int>, mppp::rational<1>>;
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
