// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <obake/hash.hpp>
#include <obake/math/truncate_degree.hpp>
#include <obake/math/truncate_p_degree.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/power_series/power_series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

TEST_CASE("basic")
{
    obake_test::disable_slow_stack_traces();

    // Type traits/concepts.
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    REQUIRE(power_series_cf<double>);
    REQUIRE(!power_series_cf<const double>);
    REQUIRE(!power_series_cf<const double &>);
    REQUIRE(!power_series_cf<void>);

    REQUIRE(power_series_key<pm_t>);
    REQUIRE(!power_series_key<const pm_t>);
    REQUIRE(!power_series_key<const pm_t &>);
    REQUIRE(!power_series_key<void>);

    REQUIRE(any_p_series<ps_t>);
    REQUIRE(!any_p_series<const ps_t>);
    REQUIRE(!any_p_series<ps_t &>);
    REQUIRE(!any_p_series<void>);

    // Default construction of the tag.
    ps_t foo;
    REQUIRE(foo.tag().trunc.get().index() == 0u);
    // Re-set the no-truncation level to trigger
    // the equality operator of no_truncation.
    foo.tag().trunc = power_series::detail::no_truncation{};
    REQUIRE(foo.tag().trunc.get().index() == 0u);

    // Tag state after truncation setting.
    set_truncation(foo, 5);
    REQUIRE(foo.tag().trunc.get().index() == 1u);
    REQUIRE(std::is_reference_v<decltype(set_truncation(foo, 5))>);
    set_truncation(foo, 5, symbol_set{"x", "y", "z"});
    REQUIRE(foo.tag().trunc.get().index() == 2u);
    REQUIRE(std::is_reference_v<decltype(set_truncation(foo, 5, symbol_set{"x", "y", "z"}))>);
    unset_truncation(foo);
    REQUIRE(foo.tag().trunc.get().index() == 0u);
    REQUIRE(std::is_reference_v<decltype(unset_truncation(foo))>);

    // Truncation getter.
    REQUIRE(get_truncation(foo).index() == 0u);
    REQUIRE(std::is_reference_v<decltype(get_truncation(foo))>);

    // Factory functions.
    {
        // No truncation, no ss.
        auto [x, y] = make_p_series<ps_t>("x", std::string{"y"});

        REQUIRE(x.size() == 1u);
        REQUIRE(x.begin()->first == pm_t{1});
        REQUIRE(x.begin()->second == 1.);
        REQUIRE(x.get_symbol_set() == symbol_set{"x"});
        REQUIRE(get_truncation(x).index() == 0u);

        REQUIRE(y.size() == 1u);
        REQUIRE(y.begin()->first == pm_t{1});
        REQUIRE(y.begin()->second == 1.);
        REQUIRE(y.get_symbol_set() == symbol_set{"y"});
        REQUIRE(get_truncation(y).index() == 0u);
    }

    {
        // No truncation, with ss.
        auto [x, z] = make_p_series<ps_t>(symbol_set{"x", "y", "z"}, std::string{"x"}, "z");

        REQUIRE(x.size() == 1u);
        REQUIRE(x.begin()->first == pm_t{1, 0, 0});
        REQUIRE(x.begin()->second == 1.);
        REQUIRE(x.get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(get_truncation(x).index() == 0u);

        REQUIRE(z.size() == 1u);
        REQUIRE(z.begin()->first == pm_t{0, 0, 1});
        REQUIRE(z.begin()->second == 1.);
        REQUIRE(z.get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(get_truncation(z).index() == 0u);

        OBAKE_REQUIRES_THROWS_CONTAINS((make_p_series<ps_t>(symbol_set{"x", "y", "z"}, std::string{"x"}, "a")),
                                       std::invalid_argument,
                                       "Cannot create a power series with symbol set {'x', 'y', 'z'} from the "
                                       "generator 'a': the generator is not in the symbol set");
    }

    {
        // Total truncation, no ss.
        auto [x, y] = make_p_series_t<ps_t>(1, "x", std::string{"y"});

        REQUIRE(x.size() == 1u);
        REQUIRE(x.begin()->first == pm_t{1});
        REQUIRE(x.begin()->second == 1.);
        REQUIRE(x.get_symbol_set() == symbol_set{"x"});
        REQUIRE(get_truncation(x).index() == 1u);
        REQUIRE(std::get<1>(get_truncation(x)) == 1);

        REQUIRE(y.size() == 1u);
        REQUIRE(y.begin()->first == pm_t{1});
        REQUIRE(y.begin()->second == 1.);
        REQUIRE(y.get_symbol_set() == symbol_set{"y"});
        REQUIRE(get_truncation(y).index() == 1u);
        REQUIRE(std::get<1>(get_truncation(y)) == 1);

        auto [a, b] = make_p_series_t<ps_t>(0, "a", std::string{"b"});

        REQUIRE(a.empty());
        REQUIRE(a.get_symbol_set() == symbol_set{"a"});
        REQUIRE(std::get<1>(get_truncation(a)) == 0);

        REQUIRE(b.empty());
        REQUIRE(b.get_symbol_set() == symbol_set{"b"});
        REQUIRE(std::get<1>(get_truncation(b)) == 0);
    }

    {
        // Total truncation, with ss.
        auto [x, y] = make_p_series_t<ps_t>(symbol_set{"x", "y", "z"}, 1, "x", std::string{"y"});

        REQUIRE(x.size() == 1u);
        REQUIRE(x.begin()->first == pm_t{1, 0, 0});
        REQUIRE(x.begin()->second == 1.);
        REQUIRE(x.get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(get_truncation(x).index() == 1u);
        REQUIRE(std::get<1>(get_truncation(x)) == 1);

        REQUIRE(y.size() == 1u);
        REQUIRE(y.begin()->first == pm_t{0, 1, 0});
        REQUIRE(y.begin()->second == 1.);
        REQUIRE(y.get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(get_truncation(y).index() == 1u);
        REQUIRE(std::get<1>(get_truncation(y)) == 1);

        auto [a, b] = make_p_series_t<ps_t>(symbol_set{"a", "b", "c"}, 0, "a", std::string{"b"});

        REQUIRE(a.empty());
        REQUIRE(a.get_symbol_set() == symbol_set{"a", "b", "c"});
        REQUIRE(std::get<1>(get_truncation(a)) == 0);

        REQUIRE(b.empty());
        REQUIRE(b.get_symbol_set() == symbol_set{"a", "b", "c"});
        REQUIRE(std::get<1>(get_truncation(b)) == 0);

        OBAKE_REQUIRES_THROWS_CONTAINS((make_p_series_t<ps_t>(symbol_set{"x", "y", "z"}, 1, std::string{"x"}, "a")),
                                       std::invalid_argument,
                                       "Cannot create a power series with symbol set {'x', 'y', 'z'} from the "
                                       "generator 'a': the generator is not in the symbol set");
    }

    {
        // Partial truncation, no ss.
        auto [x, y] = make_p_series_p<ps_t>(1, symbol_set{"x"}, "x", std::string{"y"});

        REQUIRE(x.size() == 1u);
        REQUIRE(x.begin()->first == pm_t{1});
        REQUIRE(x.begin()->second == 1.);
        REQUIRE(x.get_symbol_set() == symbol_set{"x"});
        REQUIRE(get_truncation(x).index() == 2u);
        REQUIRE(std::get<2>(get_truncation(x)) == std::pair{std::int32_t{1}, symbol_set{"x"}});

        REQUIRE(y.size() == 1u);
        REQUIRE(y.begin()->first == pm_t{1});
        REQUIRE(y.begin()->second == 1.);
        REQUIRE(y.get_symbol_set() == symbol_set{"y"});
        REQUIRE(get_truncation(y).index() == 2u);
        REQUIRE(std::get<2>(get_truncation(y)) == std::pair{std::int32_t{1}, symbol_set{"x"}});

        auto [a, b] = make_p_series_p<ps_t>(0, symbol_set{"a"}, "a", std::string{"b"});

        REQUIRE(a.empty());
        REQUIRE(a.get_symbol_set() == symbol_set{"a"});
        REQUIRE(std::get<2>(get_truncation(a)) == std::pair{std::int32_t{0}, symbol_set{"a"}});

        REQUIRE(b.size() == 1u);
        REQUIRE(b.begin()->first == pm_t{1});
        REQUIRE(b.begin()->second == 1.);
        REQUIRE(b.get_symbol_set() == symbol_set{"b"});
        REQUIRE(get_truncation(b).index() == 2u);
        REQUIRE(std::get<2>(get_truncation(b)) == std::pair{std::int32_t{0}, symbol_set{"a"}});
    }

    {
        // Partial truncation, with ss.
        auto [x, y] = make_p_series_p<ps_t>(symbol_set{"x", "y", "z"}, 1, symbol_set{"x"}, "x", std::string{"y"});

        REQUIRE(x.size() == 1u);
        REQUIRE(x.begin()->first == pm_t{1, 0, 0});
        REQUIRE(x.begin()->second == 1.);
        REQUIRE(x.get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(get_truncation(x).index() == 2u);
        REQUIRE(std::get<2>(get_truncation(x)) == std::pair{std::int32_t{1}, symbol_set{"x"}});

        REQUIRE(y.size() == 1u);
        REQUIRE(y.begin()->first == pm_t{0, 1, 0});
        REQUIRE(y.begin()->second == 1.);
        REQUIRE(y.get_symbol_set() == symbol_set{"x", "y", "z"});
        REQUIRE(get_truncation(y).index() == 2u);
        REQUIRE(std::get<2>(get_truncation(y)) == std::pair{std::int32_t{1}, symbol_set{"x"}});

        auto [a, b] = make_p_series_p<ps_t>(symbol_set{"a", "b", "c"}, 0, symbol_set{"a"}, "a", std::string{"b"});

        REQUIRE(a.empty());
        REQUIRE(a.get_symbol_set() == symbol_set{"a", "b", "c"});
        REQUIRE(std::get<2>(get_truncation(a)) == std::pair{std::int32_t{0}, symbol_set{"a"}});

        REQUIRE(b.size() == 1u);
        REQUIRE(b.begin()->first == pm_t{0, 1, 0});
        REQUIRE(b.begin()->second == 1.);
        REQUIRE(b.get_symbol_set() == symbol_set{"a", "b", "c"});
        REQUIRE(get_truncation(b).index() == 2u);
        REQUIRE(std::get<2>(get_truncation(b)) == std::pair{std::int32_t{0}, symbol_set{"a"}});

        OBAKE_REQUIRES_THROWS_CONTAINS(
            (make_p_series_p<ps_t>(symbol_set{"x", "y", "z"}, 1, symbol_set{"a"}, std::string{"x"}, "a")),
            std::invalid_argument,
            "Cannot create a power series with symbol set {'x', 'y', 'z'} from the "
            "generator 'a': the generator is not in the symbol set");
    }
}

TEST_CASE("truncate degree")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    {
        auto [x] = make_p_series<ps_t>("x");

        obake::truncate_degree(x, 0);

        REQUIRE(x.empty());

        x.add_term(pm_t{10}, 1.25);
        obake::truncate_degree(x, 10);
        REQUIRE(!x.empty());
        obake::truncate_degree(x, 9);
        REQUIRE(x.empty());
    }

    {
        auto [x] = make_p_series<ps_t>("x");

        obake::truncate_p_degree(x, 0, symbol_set{"x"});

        REQUIRE(x.empty());

        x = make_p_series<ps_t>("x")[0];
        obake::truncate_p_degree(x, 0, symbol_set{"y"});
        REQUIRE(x == make_p_series<ps_t>("x")[0]);

        x = make_p_series<ps_t>("x")[0];
        obake::truncate_p_degree(x, 0, symbol_set{});
        REQUIRE(x == make_p_series<ps_t>("x")[0]);

        x.add_term(pm_t{10}, 1.25);
        obake::truncate_p_degree(x, 10, symbol_set{"x"});
        REQUIRE(!x.empty());
        obake::truncate_p_degree(x, 0, symbol_set{"y"});
        REQUIRE(!x.empty());
        obake::truncate_p_degree(x, 0, symbol_set{"x"});
        REQUIRE(x.empty());
    }
}

TEST_CASE("explicit truncation")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    auto [x] = make_p_series<ps_t>("x");

    obake::truncate(x);

    REQUIRE(x == make_p_series<ps_t>("x")[0]);

    x = make_p_series_t<ps_t>(0, "x")[0];
    REQUIRE(x.empty());

    x.add_term(pm_t{1}, 1.25);
    REQUIRE(!x.empty());
    obake::truncate(x);
    REQUIRE(x.empty());

    x = make_p_series_p<ps_t>(0, symbol_set{"x"}, "x")[0];
    REQUIRE(x.empty());
    x.add_term(pm_t{1}, 1.25);
    REQUIRE(!x.empty());
    obake::truncate(x);
    REQUIRE(x.empty());
}

// Test that the tag is taken into
// account when comparing.
TEST_CASE("comparison")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    auto [x] = make_p_series<ps_t>("x");

    REQUIRE(x == x);
    REQUIRE(!(x != x));

    auto xt(x);

    REQUIRE(x == xt);
    REQUIRE(!(x != xt));
    REQUIRE(xt == x);
    REQUIRE(!(xt != x));

    obake::set_truncation(xt, 3);

    REQUIRE(!(x == xt));
    REQUIRE(x != xt);
    REQUIRE(!(xt == x));
    REQUIRE(xt != x);

    obake::set_truncation(xt, 3, symbol_set{"a", "b"});

    REQUIRE(!(x == xt));
    REQUIRE(x != xt);
    REQUIRE(!(xt == x));
    REQUIRE(xt != x);

    obake::unset_truncation(xt);

    REQUIRE(x == xt);
    REQUIRE(!(x != xt));
    REQUIRE(xt == x);
    REQUIRE(!(xt != x));
}

TEST_CASE("s11n")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    {
        auto [x] = make_p_series<ps_t>("x");

        std::stringstream oss;
        {
            boost::archive::binary_oarchive oa(oss);

            oa << x;
        }

        x = make_p_series<ps_t>("y")[0];

        {
            boost::archive::binary_iarchive ia(oss);

            ia >> x;
        }

        REQUIRE(x == make_p_series<ps_t>("x")[0]);
        REQUIRE(obake::get_truncation(x).index() == 0u);
    }

    {
        auto [x] = make_p_series<ps_t>("x");

        std::stringstream oss;
        {
            boost::archive::binary_oarchive oa(oss);

            oa << x;
        }

        x = make_p_series_t<ps_t>(1, "y")[0];

        {
            boost::archive::binary_iarchive ia(oss);

            ia >> x;
        }

        REQUIRE(x == make_p_series<ps_t>("x")[0]);
    }

    {
        auto [x] = make_p_series<ps_t>("x");

        std::stringstream oss;
        {
            boost::archive::binary_oarchive oa(oss);

            oa << x;
        }

        x = make_p_series_p<ps_t>(1, symbol_set{"a"}, "y")[0];

        {
            boost::archive::binary_iarchive ia(oss);

            ia >> x;
        }

        REQUIRE(x == make_p_series<ps_t>("x")[0]);
    }

    {
        auto [x] = make_p_series_t<ps_t>(42, "x");

        std::stringstream oss;
        {
            boost::archive::binary_oarchive oa(oss);

            oa << x;
        }

        x = make_p_series_p<ps_t>(1, symbol_set{"a"}, "y")[0];

        {
            boost::archive::binary_iarchive ia(oss);

            ia >> x;
        }

        REQUIRE(x == make_p_series_t<ps_t>(42, "x")[0]);
        REQUIRE(obake::get_truncation(x).index() == 1u);
        REQUIRE(std::get<1>(obake::get_truncation(x)) == 42);
    }

    {
        auto [x] = make_p_series_p<ps_t>(42, symbol_set{"x"}, "x");

        std::stringstream oss;
        {
            boost::archive::binary_oarchive oa(oss);

            oa << x;
        }

        x = make_p_series_p<ps_t>(1, symbol_set{"a"}, "y")[0];

        {
            boost::archive::binary_iarchive ia(oss);

            ia >> x;
        }

        REQUIRE(x == make_p_series_p<ps_t>(42, symbol_set{"x"}, "x")[0]);
        REQUIRE(obake::get_truncation(x).index() == 2u);
        REQUIRE(std::get<2>(obake::get_truncation(x)) == std::pair{42, symbol_set{"x"}});
    }
}

// Make sure swapping is working as expected.
TEST_CASE("swap")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    REQUIRE(std::is_nothrow_swappable_v<ps_t>);
    REQUIRE(std::is_nothrow_swappable_v<remove_cvref_t<decltype(ps_t{}.tag())>>);

    auto [x] = make_p_series<ps_t>("x");
    auto [y] = make_p_series_t<ps_t>(10, "y");

    using std::swap;

    swap(x, y);

    REQUIRE(x == make_p_series_t<ps_t>(10, "y")[0]);
    REQUIRE(obake::get_truncation(x).index() == 1u);
    REQUIRE(std::get<1>(obake::get_truncation(x)) == 10);
    REQUIRE(x.get_symbol_set() == symbol_set{"y"});

    REQUIRE(y == make_p_series<ps_t>("x")[0]);
    REQUIRE(obake::get_truncation(y).index() == 0u);
    REQUIRE(y.get_symbol_set() == symbol_set{"x"});
}

// Make sure clear() is working as expected.
TEST_CASE("clear")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    auto [y] = make_p_series_t<ps_t>(10, "y");
    REQUIRE(obake::get_truncation(y).index() == 1u);

    y.clear();

    REQUIRE(y.empty());
    REQUIRE(y.get_symbol_set() == symbol_set{});
    REQUIRE(obake::get_truncation(y).index() == 0u);
}

// Make sure hash() is working as expected.
// TODO will need to do testing for pow caching,
// once we have the multiplication.
TEST_CASE("hash")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    REQUIRE(is_hashable_v<const remove_cvref_t<decltype(ps_t{}.tag())> &>);

    auto [x, y] = make_p_series<ps_t>("x", "y");

    REQUIRE(x.tag() == y.tag());
    REQUIRE(::obake::hash(x.tag()) == ::obake::hash(y.tag()));

    auto [a, b] = make_p_series_t<ps_t>(23, "x", "y");

    REQUIRE(a.tag() == a.tag());
    REQUIRE(::obake::hash(a.tag()) == ::obake::hash(a.tag()));

    auto [s, t] = make_p_series_p<ps_t>(23, symbol_set{"a", "b"}, "x", "y");

    REQUIRE(s.tag() == t.tag());
    REQUIRE(::obake::hash(s.tag()) == ::obake::hash(t.tag()));

    REQUIRE(x.tag() != a.tag());
    REQUIRE(x.tag() != s.tag());
    REQUIRE(a.tag() != s.tag());
}

TEST_CASE("stream operator")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    {
        std::ostringstream oss;

        auto [x] = make_p_series<ps_t>("x");

        oss << x;

        REQUIRE(boost::contains(oss.str(), "power series"));
        REQUIRE(boost::contains(oss.str(), "Truncation: none"));
    }

    {
        std::ostringstream oss;

        auto [x] = make_p_series_t<ps_t>(10, "x");

        oss << x;

        REQUIRE(boost::contains(oss.str(), "power series"));
        REQUIRE(boost::contains(oss.str(), "Truncation degree: 10"));
    }

    {
        std::ostringstream oss;

        auto [x] = make_p_series_p<ps_t>(10, symbol_set{"a"}, "x");

        oss << x;

        REQUIRE(boost::contains(oss.str(), "power series"));
        REQUIRE(boost::contains(oss.str(), "Partial truncation degree: 10, {'a'}"));
    }
}

TEST_CASE("poly conversion")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;
    using poly_t = polynomial<pm_t, double>;

    {
        auto [x] = make_p_series_t<ps_t>(symbol_set{"x", "y"}, 10, "x");

        poly_t xp(x);

        REQUIRE(xp == make_polynomials<poly_t>("x")[0]);
        REQUIRE(xp.get_symbol_set() == symbol_set{"x", "y"});
    }
    {
        auto [x] = make_polynomials<poly_t>(symbol_set{"x", "y"}, "x");

        ps_t xp(x);

        REQUIRE(xp == make_p_series<ps_t>("x")[0]);
        REQUIRE(xp.get_symbol_set() == symbol_set{"x", "y"});
        REQUIRE(obake::get_truncation(xp).index() == 0u);
    }
}

// Test to check the tag is correctly preserved
// when converting between p_series with different
// coefficient types.
TEST_CASE("tag preserve")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps1_t = p_series<pm_t, double>;
    using ps2_t = p_series<pm_t, float>;

    auto [x] = make_p_series_p<ps1_t>(symbol_set{"a", "x"}, 10, symbol_set{"x", "y"}, "x");

    ps2_t x2(x);

    REQUIRE(x2.get_symbol_set() == symbol_set{"a", "x"});
    REQUIRE(x2.size() == 1);
    REQUIRE(x2.begin()->first == pm_t{0, 1});
    REQUIRE(x2.begin()->second == 1);
    REQUIRE(get_truncation(x2).index() == 2u);
    REQUIRE(x2.tag() == x.tag());
}

TEST_CASE("add")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    auto check_ret_00 = [](const auto &ret) {
        REQUIRE(std::all_of(ret.begin(), ret.end(), [](const auto &t) { return t.second == 1; }));
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.first == pm_t{1, 0}; }));
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.first == pm_t{0, 1}; }));
    };

    {
        auto [x, y] = make_p_series<ps_t>("x", "y");

        auto ret = x + y;

        REQUIRE(std::is_same_v<decltype(ret), ps_t>);
        REQUIRE(ret.get_symbol_set() == symbol_set{"x", "y"});
        REQUIRE(ret.size() == 2u);
        REQUIRE(get_truncation(ret).index() == 0u);
        check_ret_00(ret);
    }

    {
        auto [x, y] = make_p_series_t<ps_t>(3, "x", "y");

        auto ret = x + y;

        REQUIRE(std::is_same_v<decltype(ret), ps_t>);
        REQUIRE(ret.get_symbol_set() == symbol_set{"x", "y"});
        REQUIRE(ret.size() == 2u);
        REQUIRE(get_truncation(ret).index() == 1u);
        check_ret_00(ret);
    }

    {
        auto [x, y] = make_p_series_p<ps_t>(3, symbol_set{"a", "b"}, "x", "y");

        auto ret = x + y;

        REQUIRE(std::is_same_v<decltype(ret), ps_t>);
        REQUIRE(ret.get_symbol_set() == symbol_set{"x", "y"});
        REQUIRE(ret.size() == 2u);
        REQUIRE(get_truncation(ret).index() == 2u);
        check_ret_00(ret);
    }

    // Conflicting truncation levels.
    {
        auto [x] = make_p_series_t<ps_t>(3, "x");
        auto [y] = make_p_series_t<ps_t>(2, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x + y, std::invalid_argument,
                                       "Unable to add two power series if their truncation levels do not match");
    }
    {
        auto [x] = make_p_series<ps_t>("x");
        auto [y] = make_p_series_t<ps_t>(2, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x + y, std::invalid_argument,
                                       "Unable to add two power series if their truncation levels do not match");
    }
    {
        auto [x] = make_p_series<ps_t>("x");
        auto [y] = make_p_series_p<ps_t>(2, symbol_set{"a"}, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x + y, std::invalid_argument,
                                       "Unable to add two power series if their truncation levels do not match");
    }

    // Tests with non-series operand.
    auto check_ret_01 = []<typename T>(const T &ret) {
        REQUIRE(ret.get_symbol_set() == symbol_set{"x"});
        REQUIRE(std::is_same_v<T, ps_t>);
        REQUIRE(obake::get_truncation(ret).index() == 1u);
        REQUIRE(std::get<1>(obake::get_truncation(ret)) == 3);
        REQUIRE(ret.size() == 2u);
        REQUIRE(std::all_of(ret.begin(), ret.end(), [](const auto &t) { return t.second == 1; }));
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.first == pm_t{0}; }));
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.first == pm_t{1}; }));
    };

    using ps2_t = p_series<pm_t, float>;
    {
        auto [x] = make_p_series_t<ps2_t>(3, "x");

        check_ret_01(x + 1.);
        check_ret_01(1. + x);
    }
    {
        auto [x] = make_p_series_t<ps_t>(3, "x");

        check_ret_01(x + 1);
        check_ret_01(1 + x);
    }
    {
        // Check with effective truncation.
        auto [x] = make_p_series_t<ps2_t>(-1, "x");

        REQUIRE(x.empty());
        REQUIRE((x + 1.).empty());
        REQUIRE((1. + x).empty());
    }
    {
        auto [x] = make_p_series_t<ps_t>(-1, "x");

        REQUIRE(x.empty());
        REQUIRE((x + 1).empty());
        REQUIRE((1 + x).empty());
    }
}

TEST_CASE("sub")
{
    using pm_t = packed_monomial<std::int32_t>;
    using ps_t = p_series<pm_t, double>;

    auto check_ret_00 = [](const auto &ret) {
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.second == 1; }));
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.second == -1; }));
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.first == pm_t{1, 0}; }));
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.first == pm_t{0, 1}; }));
    };

    {
        auto [x, y] = make_p_series<ps_t>("x", "y");

        auto ret = x - y;

        REQUIRE(std::is_same_v<decltype(ret), ps_t>);
        REQUIRE(ret.get_symbol_set() == symbol_set{"x", "y"});
        REQUIRE(ret.size() == 2u);
        REQUIRE(get_truncation(ret).index() == 0u);
        check_ret_00(ret);
    }

    {
        auto [x, y] = make_p_series_t<ps_t>(3, "x", "y");

        auto ret = x - y;

        REQUIRE(std::is_same_v<decltype(ret), ps_t>);
        REQUIRE(ret.get_symbol_set() == symbol_set{"x", "y"});
        REQUIRE(ret.size() == 2u);
        REQUIRE(get_truncation(ret).index() == 1u);
        check_ret_00(ret);
    }

    {
        auto [x, y] = make_p_series_p<ps_t>(3, symbol_set{"a", "b"}, "x", "y");

        auto ret = x - y;

        REQUIRE(std::is_same_v<decltype(ret), ps_t>);
        REQUIRE(ret.get_symbol_set() == symbol_set{"x", "y"});
        REQUIRE(ret.size() == 2u);
        REQUIRE(get_truncation(ret).index() == 2u);
        check_ret_00(ret);
    }

    // Conflicting truncation levels.
    {
        auto [x] = make_p_series_t<ps_t>(3, "x");
        auto [y] = make_p_series_t<ps_t>(2, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x - y, std::invalid_argument,
                                       "Unable to subtract two power series if their truncation levels do not match");
    }
    {
        auto [x] = make_p_series<ps_t>("x");
        auto [y] = make_p_series_t<ps_t>(2, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x - y, std::invalid_argument,
                                       "Unable to subtract two power series if their truncation levels do not match");
    }
    {
        auto [x] = make_p_series<ps_t>("x");
        auto [y] = make_p_series_p<ps_t>(2, symbol_set{"a"}, "y");

        OBAKE_REQUIRES_THROWS_CONTAINS(x - y, std::invalid_argument,
                                       "Unable to subtract two power series if their truncation levels do not match");
    }

    // Tests with non-series operand.
    auto check_ret_01 = []<typename T>(const T &ret) {
        REQUIRE(ret.get_symbol_set() == symbol_set{"x"});
        REQUIRE(std::is_same_v<T, ps_t>);
        REQUIRE(obake::get_truncation(ret).index() == 1u);
        REQUIRE(std::get<1>(obake::get_truncation(ret)) == 3);
        REQUIRE(ret.size() == 2u);
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.second == 1; }));
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.second == -1; }));
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.first == pm_t{0}; }));
        REQUIRE(std::any_of(ret.begin(), ret.end(), [](const auto &t) { return t.first == pm_t{1}; }));
    };

    using ps2_t = p_series<pm_t, float>;
    {
        auto [x] = make_p_series_t<ps2_t>(3, "x");

        check_ret_01(x - 1.);
        check_ret_01(1. - x);
    }
    {
        auto [x] = make_p_series_t<ps_t>(3, "x");

        check_ret_01(x - 1);
        check_ret_01(1 - x);
    }
    {
        // Check with effective truncation.
        auto [x] = make_p_series_t<ps2_t>(-1, "x");

        REQUIRE(x.empty());
        REQUIRE((x - 1.).empty());
        REQUIRE((1. - x).empty());
    }
    {
        auto [x] = make_p_series_t<ps_t>(-1, "x");

        REQUIRE(x.empty());
        REQUIRE((x - 1).empty());
        REQUIRE((1 - x).empty());
    }
}
