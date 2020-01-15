// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <sstream>
#include <utility>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <obake/key/key_degree.hpp>
#include <obake/math/degree.hpp>
#include <obake/math/pow.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/s11n.hpp>
#include <obake/symbols.hpp>
#include <obake/tex_stream_insert.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"

using int_t = mppp::integer<1>;
using rat_t = mppp::rational<1>;

using namespace obake;

TEST_CASE("series_tex_stream_test")
{
    using pm_t = packed_monomial<int>;
    using p1_t = polynomial<pm_t, rat_t>;
    using p11_t = polynomial<pm_t, p1_t>;
    using p2_t = polynomial<pm_t, int_t>;

    auto [x, y, z] = make_polynomials<p1_t>("x", "y", "z");

    std::ostringstream oss;

    tex_stream_insert(oss, p1_t{});
    REQUIRE(oss.str() == "0");
    oss.str("");

    tex_stream_insert(oss, p1_t{rat_t{1, 2}});
    REQUIRE(oss.str() == "\\frac{1}{2}");
    oss.str("");

    tex_stream_insert(oss, x / 2);
    REQUIRE(oss.str() == "\\frac{1}{2}{x}");
    oss.str("");

    tex_stream_insert(oss, x * x * y * z / 2);
    REQUIRE(oss.str() == "\\frac{1}{2}{x}^{2}{y}{z}");
    oss.str("");

    tex_stream_insert(oss, -x * x * y * z / 2);
    REQUIRE(oss.str() == "-\\frac{1}{2}{x}^{2}{y}{z}");
    oss.str("");

    tex_stream_insert(oss, -x * x * y * obake::pow(z, -5) / 2);
    REQUIRE(oss.str() == "-\\frac{1}{2}\\frac{{x}^{2}{y}}{{z}^{5}}");
    oss.str("");

    tex_stream_insert(oss, -x * x * y * obake::pow(z, -5) / 2 + x * y * z / 6);
    REQUIRE((oss.str() == "-\\frac{1}{2}\\frac{{x}^{2}{y}}{{z}^{5}}+\\frac{1}{6}{x}{y}{z}"
             || oss.str() == "\\frac{1}{6}{x}{y}{z}-\\frac{1}{2}\\frac{{x}^{2}{y}}{{z}^{5}}"));
    oss.str("");

    // Try exceeding the limit.
    p1_t p;
    for (int i = 0; i < 100; ++i) {
        p += obake::pow(x, i);
    }
    tex_stream_insert(oss, p);
    REQUIRE(boost::ends_with(oss.str(), "\\ldots"));
    oss.str("");

    // A couple of tests with coefficients that do not
    // have a specialised tex representation.
    auto [a, b, c] = make_polynomials<p2_t>("a", "b", "c");

    tex_stream_insert(oss, p2_t{});
    REQUIRE(oss.str() == "0");
    oss.str("");

    tex_stream_insert(oss, p2_t{-42});
    REQUIRE(oss.str() == "-42");
    oss.str("");

    tex_stream_insert(oss, a * 10);
    REQUIRE(oss.str() == "10{a}");
    oss.str("");

    tex_stream_insert(oss, -a * a * b * obake::pow(c, 5) * 4 + 3 * a * b * c);
    REQUIRE((oss.str() == "-4{a}^{2}{b}{c}^{5}+3{a}{b}{c}" || oss.str() == "3{a}{b}{c}-4{a}^{2}{b}{c}^{5}"));
    oss.str("");

    p2_t q;
    for (int i = 0; i < 100; ++i) {
        q += obake::pow(a, i);
    }
    tex_stream_insert(oss, q);
    REQUIRE(boost::ends_with(oss.str(), "\\ldots"));
    oss.str("");

    // Nested polys.
    auto [t, u] = make_polynomials<p11_t>("t", "u");

    tex_stream_insert(oss, p11_t{});
    REQUIRE(oss.str() == "0");
    oss.str("");

    tex_stream_insert(oss, p11_t{rat_t{1, 2}});
    REQUIRE(oss.str() == "\\frac{1}{2}");
    oss.str("");

    tex_stream_insert(oss, t / 2);
    REQUIRE(oss.str() == "\\frac{1}{2}{t}");
    oss.str("");

    tex_stream_insert(oss, t * t * u / 2);
    REQUIRE(oss.str() == "\\frac{1}{2}{t}^{2}{u}");
    oss.str("");

    tex_stream_insert(oss, 2 * (x * x * y * z / 3) * t * t * u / 2);
    REQUIRE(oss.str() == "\\frac{1}{3}{x}^{2}{y}{z}{t}^{2}{u}");
    oss.str("");

    tex_stream_insert(oss, 2 * (x * x * y * z / 3 - x * y * z * z) * t * t * u / 2);
    REQUIRE((oss.str() == "\\left(\\frac{1}{3}{x}^{2}{y}{z}-{x}{y}{z}^{2}\\right){t}^{2}{u}"
             || oss.str() == "\\left(-{x}{y}{z}^{2}+\\frac{1}{3}{x}^{2}{y}{z}\\right){t}^{2}{u}"));
    oss.str("");
}

template <typename T, typename F>
using filter_t = decltype(filter(std::declval<T>(), std::declval<F>()));

TEST_CASE("series_filter_test")
{
    using pm_t = packed_monomial<int>;
    using p1_t = polynomial<pm_t, rat_t>;

    p1_t tmp;
    filter(tmp, [](const auto &) { return true; });
    REQUIRE(tmp.empty());

    tmp.set_symbol_set(symbol_set{"a", "b", "c"});
    tmp.set_n_segments(4);

    auto tmp_f(tmp);
    filter(tmp_f, [](const auto &) { return true; });
    REQUIRE(tmp_f.empty());
    REQUIRE(tmp_f.get_symbol_set() == symbol_set{"a", "b", "c"});
    REQUIRE(tmp_f._get_s_table().size() == 16u);

    auto [x, y, z] = make_polynomials<p1_t>("x", "y", "z");

    auto p = obake::pow(1 + x + y + z, 4);
    auto pf(p);
    filter(pf, [&ss = p.get_symbol_set()](const auto &t) { return obake::key_degree(t.first, ss) <= 1; });
    REQUIRE(obake::degree(pf) == 1);
    pf = p;
    filter(pf, [&ss = p.get_symbol_set()](const auto &t) { return obake::key_degree(t.first, ss) <= 2; });
    REQUIRE(obake::degree(pf) == 2);
    pf = p;
    filter(pf, [&ss = p.get_symbol_set()](const auto &t) { return obake::key_degree(t.first, ss) <= 3; });
    REQUIRE(obake::degree(pf) == 3);
    REQUIRE(pf.get_symbol_set() == symbol_set{"x", "y", "z"});

    REQUIRE(!is_detected_v<filter_t, void, void>);
    REQUIRE(!is_detected_v<filter_t, void, int>);
    REQUIRE(!is_detected_v<filter_t, int, void>);
    auto good_f = [](const auto &) { return true; };
    REQUIRE(!is_detected_v<filter_t, int, decltype(good_f)>);
    REQUIRE(!is_detected_v<filter_t, p1_t, decltype(good_f)>);
    REQUIRE(!is_detected_v<filter_t, const p1_t &, decltype(good_f)>);
    REQUIRE(is_detected_v<filter_t, p1_t &, decltype(good_f)>);
    REQUIRE(!is_detected_v<filter_t, p1_t, decltype(good_f) &>);
    REQUIRE(!is_detected_v<filter_t, const p1_t &, decltype(good_f) &>);
    REQUIRE(is_detected_v<filter_t, p1_t &, decltype(good_f) &>);
    REQUIRE(!is_detected_v<filter_t, p1_t, const decltype(good_f) &>);
    REQUIRE(!is_detected_v<filter_t, const p1_t &, const decltype(good_f) &>);
    REQUIRE(is_detected_v<filter_t, p1_t &, const decltype(good_f) &>);
    auto good_f1 = [](const auto &) { return 1; };
    REQUIRE(!is_detected_v<filter_t, p1_t, decltype(good_f1)>);
    REQUIRE(!is_detected_v<filter_t, p1_t, decltype(good_f1) &>);
    REQUIRE(!is_detected_v<filter_t, p1_t, const decltype(good_f1) &>);
    REQUIRE(!is_detected_v<filter_t, const p1_t &, decltype(good_f1)>);
    REQUIRE(!is_detected_v<filter_t, const p1_t &, decltype(good_f1) &>);
    REQUIRE(!is_detected_v<filter_t, const p1_t &, const decltype(good_f1) &>);
    REQUIRE(is_detected_v<filter_t, p1_t &, decltype(good_f1)>);
    REQUIRE(is_detected_v<filter_t, p1_t &, decltype(good_f1) &>);
    REQUIRE(is_detected_v<filter_t, p1_t &, const decltype(good_f1) &>);
    auto bad_f0 = []() { return true; };
    REQUIRE(!is_detected_v<filter_t, p1_t &, decltype(bad_f0)>);
    auto bad_f1 = [](const auto &) {};
    REQUIRE(!is_detected_v<filter_t, p1_t &, decltype(bad_f1)>);
    struct foo {
    };
    auto bad_f2 = [](const auto &) { return foo{}; };
    REQUIRE(!is_detected_v<filter_t, p1_t &, decltype(bad_f2)>);
}

template <typename T>
using add_symbols_t = decltype(add_symbols(std::declval<T>(), std::declval<const symbol_set &>()));

TEST_CASE("series_add_symbols_test")
{
    using pm_t = packed_monomial<int>;
    using p1_t = polynomial<pm_t, rat_t>;

    REQUIRE(add_symbols(p1_t{}, symbol_set{}).empty());
    REQUIRE(add_symbols(p1_t{}, symbol_set{}).get_symbol_set() == symbol_set{});

    auto [x, y, z] = make_polynomials<p1_t>("x", "y", "z");

    REQUIRE(add_symbols(x, symbol_set{"x"}) == x);
    REQUIRE(add_symbols(x, symbol_set{"x"}).get_symbol_set() == symbol_set{"x"});
    REQUIRE(add_symbols(x, symbol_set{"x", "y"}) == x);
    REQUIRE(add_symbols(x, symbol_set{"x", "y"}).get_symbol_set() == symbol_set{"x", "y"});

    auto p = obake::pow(1 + x + y + z, 4);
    REQUIRE(add_symbols(p, symbol_set{}) == p);
    REQUIRE(add_symbols(p, symbol_set{}).get_symbol_set() == symbol_set{"x", "y", "z"});
    REQUIRE(add_symbols(p, symbol_set{"x"}) == p);
    REQUIRE(add_symbols(p, symbol_set{"x"}).get_symbol_set() == symbol_set{"x", "y", "z"});
    REQUIRE(add_symbols(p, symbol_set{"x", "y"}) == p);
    REQUIRE(add_symbols(p, symbol_set{"x", "y"}).get_symbol_set() == symbol_set{"x", "y", "z"});
    REQUIRE(add_symbols(p, symbol_set{"x", "y", "z"}) == p);
    REQUIRE(add_symbols(p, symbol_set{"x", "y", "z"}).get_symbol_set() == symbol_set{"x", "y", "z"});
    REQUIRE(add_symbols(p, symbol_set{"t"}) == p);
    REQUIRE(add_symbols(p, symbol_set{"t"}).get_symbol_set() == symbol_set{"t", "x", "y", "z"});
    REQUIRE(add_symbols(p, symbol_set{"t", "x", "y"}) == p);
    REQUIRE(add_symbols(p, symbol_set{"t", "x", "y"}).get_symbol_set() == symbol_set{"t", "x", "y", "z"});
    REQUIRE(add_symbols(p, symbol_set{"t", "x", "y", "z"}) == p);
    REQUIRE(add_symbols(p, symbol_set{"t", "x", "y", "z"}).get_symbol_set() == symbol_set{"t", "x", "y", "z"});

    REQUIRE(!is_detected_v<add_symbols_t, void>);
    REQUIRE(!is_detected_v<add_symbols_t, const void>);
    REQUIRE(!is_detected_v<add_symbols_t, int>);
    REQUIRE(is_detected_v<add_symbols_t, p1_t>);
    REQUIRE(is_detected_v<add_symbols_t, p1_t &>);
    REQUIRE(is_detected_v<add_symbols_t, const p1_t &>);
}

#if !defined(_MSC_VER) || defined(__clang__)

TEST_CASE("series_s11n_test")
{
    using pm_t = packed_monomial<int>;
    using p1_t = polynomial<pm_t, double>;

    REQUIRE(boost::serialization::tracking_level<p1_t>::value == boost::serialization::track_never);

    std::stringstream ss;
    p1_t tmp;

    {
        boost::archive::binary_oarchive oarchive(ss);
        oarchive << p1_t{};
    }
    {
        boost::archive::binary_iarchive iarchive(ss);
        iarchive >> tmp;
    }
    REQUIRE(tmp.empty());
    ss.str("");

    auto [x, y, z] = make_polynomials<p1_t>("x", "y", "z");

    {
        boost::archive::binary_oarchive oarchive(ss);
        oarchive << obake::pow(x - 2 * y + 3 * z, 4);
    }
    {
        boost::archive::binary_iarchive iarchive(ss);
        iarchive >> tmp;
    }
    REQUIRE(tmp == obake::pow(x - 2 * y + 3 * z, 4));
    ss.str("");

    p1_t tmp2;
    // Check with segmentation as well.
    tmp2.set_symbol_set(symbol_set{"x", "y", "z"});
    tmp2.set_n_segments(3);
    tmp2.add_term(pm_t{1, 0, 0}, 1);
    tmp2.add_term(pm_t{0, 2, 0}, 2);
    tmp2.add_term(pm_t{0, 0, 3}, 3);

    {
        boost::archive::binary_oarchive oarchive(ss);
        oarchive << tmp2;
    }
    {
        boost::archive::binary_iarchive iarchive(ss);
        iarchive >> tmp;
    }
    REQUIRE(tmp == tmp2);
    REQUIRE(tmp.get_s_size() == 3);
    ss.str("");
}

TEST_CASE("series_table_stats_test")
{
    using pm_t = packed_monomial<int>;
    using p1_t = polynomial<pm_t, double>;

    REQUIRE(!boost::contains(p1_t{}.table_stats(), "Average terms per table"));

    auto [x, y, z] = make_polynomials<p1_t>("x", "y", "z");

    REQUIRE(boost::contains(obake::pow(x + y + z, 5).table_stats(), "Average terms per table"));
}

#endif
