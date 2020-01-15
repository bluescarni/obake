// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <initializer_list>
#include <ostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include <mp++/rational.hpp>

#include <obake/config.hpp>
#include <obake/detail/ignore.hpp>
#include <obake/math/is_zero.hpp>
#include <obake/math/negate.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using rat_t = mppp::rational<1>;

using namespace obake;

std::mt19937 rng;

const auto ntrials = 200;

TEST_CASE("series_is_single_cf")
{
    obake_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    REQUIRE(s1_t{}.is_single_cf());
    REQUIRE(s1_t{"3/4"}.is_single_cf());
    s1_t s1;
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, "3/4");
    REQUIRE(!s1.is_single_cf());
}

TEST_CASE("series_set_symbol_set")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    REQUIRE(s1.get_symbol_set() == symbol_set{"x", "y", "z"});

    s1 = s1_t{"3/4"};
    OBAKE_REQUIRES_THROWS_CONTAINS(s1.set_symbol_set(symbol_set{}), std::invalid_argument,
                                   "A symbol set can be set only in an empty series, but this series has 1 terms");
}

TEST_CASE("series_reserve")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    s1.reserve(42);
    REQUIRE(s1._get_s_table().size() == 1u);
    REQUIRE(s1._get_s_table()[0].bucket_count() != 0u);

    s1 = s1_t{};
    s1.set_n_segments(2);
    s1.reserve(32);
    REQUIRE(s1._get_s_table().size() == 4u);
    REQUIRE(s1._get_s_table()[0].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[1].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[2].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[3].bucket_count() != 0u);

    s1 = s1_t{};
    s1.set_n_segments(2);
    s1.reserve(37);
    REQUIRE(s1._get_s_table().size() == 4u);
    REQUIRE(s1._get_s_table()[0].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[1].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[2].bucket_count() != 0u);
    REQUIRE(s1._get_s_table()[3].bucket_count() != 0u);
}

TEST_CASE("series_set_n_segments")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    s1.set_n_segments(0);
    REQUIRE(s1._get_s_table().size() == 1u);
    s1.set_n_segments(1);
    REQUIRE(s1._get_s_table().size() == 2u);
    s1.set_n_segments(2);
    REQUIRE(s1._get_s_table().size() == 4u);
    s1.set_n_segments(4);
    REQUIRE(s1._get_s_table().size() == 16u);
    OBAKE_REQUIRES_THROWS_CONTAINS(s1.set_n_segments(s1.get_max_s_size() + 1u), std::invalid_argument,
                                   " as this value exceeds the maximum allowed value");
}

TEST_CASE("series_clear")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1;
    s1.set_n_segments(2);
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, 1);
    s1.add_term(pm_t{-1, -2, -3}, -1);
    s1.add_term(pm_t{4, 5, 6}, 2);
    s1.add_term(pm_t{7, 8, 9}, -2);
    s1.clear();

    REQUIRE(s1.empty());
    REQUIRE(s1.get_symbol_set() == symbol_set{});
}

TEST_CASE("series_unary_plus")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1{"3/4"};
    auto s1_c(+s1);
    REQUIRE(s1_c.size() == 1u);
    REQUIRE(s1_c.begin()->second == rat_t{3, 4});

    const auto &ptr = &(s1.begin()->second);
    auto s1_c2(+std::move(s1));
    REQUIRE(s1_c2.size() == 1u);
    REQUIRE(s1_c2.begin()->second == rat_t{3, 4});
    REQUIRE(&(s1_c2.begin()->second) == ptr);
}

TEST_CASE("series_unary_minus")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1{"3/4"};
    auto s1_c(-s1);
    REQUIRE(s1_c.size() == 1u);
    REQUIRE(s1_c.begin()->second == -rat_t{3, 4});

    const auto &ptr = &(s1.begin()->second);
    auto s1_c2(-std::move(s1));
    REQUIRE(s1_c2.size() == 1u);
    REQUIRE(s1_c2.begin()->second == -rat_t{3, 4});
    REQUIRE(&(s1_c2.begin()->second) == ptr);
}

TEST_CASE("series_negate")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    s1_t s1{"3/4"};
    const auto &ptr = &(s1.begin()->second);
    negate(s1);
    REQUIRE(s1.begin()->second == -rat_t{3, 4});
    REQUIRE(&(s1.begin()->second) == ptr);
    negate(std::move(s1));
    REQUIRE(s1.begin()->second == rat_t{3, 4});
    REQUIRE(&(s1.begin()->second) == ptr);

    REQUIRE(!is_negatable_v<const s1_t &>);
    REQUIRE(!is_negatable_v<const s1_t &&>);
}

TEST_CASE("series_is_zero")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;

    REQUIRE(is_zero(s1_t{}));
    REQUIRE(is_zero(s1_t{0}));
    REQUIRE(!is_zero(s1_t{"3/4"}));

    s1_t s1;
    REQUIRE(is_zero(s1));
    s1 = s1_t{4};
    REQUIRE(!is_zero(s1));
}

TEST_CASE("series_stream_insert_default_impl")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using s2_t = series<pm_t, s1_t, void>;

    std::ostringstream oss;

    // Empty series.
    s1_t s1;
    oss << s1;
    REQUIRE(boost::contains(oss.str(), "\n0"));

    oss.str("");
    s1.set_symbol_set(symbol_set{"x"});
    s1.add_term(pm_t{3}, "3/4");
    oss << s1;
    REQUIRE(boost::contains(oss.str(), "3/4*x**3"));

    s1.add_term(pm_t{1}, "1/2");
    oss.str("");
    oss << s1;
    REQUIRE(boost::contains(oss.str(), "1/2*x"));

    // Unitary coefficient, non-unitary key.
    s1.add_term(pm_t{7}, "1");
    oss.str("");
    oss << s1;
    REQUIRE(boost::contains(oss.str(), "x**7"));

    // Negative unitary coefficient, non-unitary key.
    s1.add_term(pm_t{6}, "-1");
    oss.str("");
    oss << s1;
    REQUIRE(boost::contains(oss.str(), "-x**6"));

    // Non-unitary coefficient, non-unitary key.
    s1.add_term(pm_t{10}, "3/2");
    oss.str("");
    oss << s1;
    REQUIRE(boost::contains(oss.str(), "3/2*x**10"));

    // Test the ellipsis.
    auto s1_old(s1);
    s1 = s1_t{};
    s1.set_symbol_set(symbol_set{"x"});
    for (auto i = 0u; i < 100u; ++i) {
        if (i % 2u) {
            s1.add_term(pm_t{static_cast<int>(i)}, static_cast<int>(i));
        } else {
            s1.add_term(pm_t{static_cast<int>(i)}, -static_cast<int>(i));
        }
    }
    oss.str("");
    oss << s1;
    REQUIRE(boost::contains(oss.str(), "..."));

    // Test rank-2 series.
    s1 = s1_old;
    s2_t s2;
    s2.set_symbol_set(symbol_set{"y"});
    s2.add_term(pm_t{-2}, s1);
    oss.str("");
    oss << s2;
    // If the coefficient series contains more than 1 term,
    // then it will be enclosed in round brackets when printed.
    REQUIRE(boost::contains(oss.str(), "("));
    REQUIRE(boost::contains(oss.str(), ")"));

    // Print them to screen for visual debug.
    std::cout << s1 << '\n';
    std::cout << s2 << '\n';

    s1 = s1_t{};
    s1.set_symbol_set(symbol_set{"x"});
    s1.add_term(pm_t{3}, "3/4");
    s2 = s2_t{};
    s2.set_symbol_set(symbol_set{"y"});
    s2.add_term(pm_t{-2}, s1);
    oss.str("");
    oss << s2;
    REQUIRE(!boost::contains(oss.str(), "("));
    REQUIRE(!boost::contains(oss.str(), ")"));
}

namespace ns
{

using pm_t = packed_monomial<int>;

// ADL-based customization.
struct tag00 {
};

inline void series_stream_insert(std::ostream &os, const series<pm_t, rat_t, tag00> &)
{
    os << "Hello world!";
}

// External customisation.
struct tag01 {
};

using s1_t = series<pm_t, rat_t, tag01>;

} // namespace ns

namespace obake::customisation
{

template <typename T>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, ns::s1_t> inline constexpr auto series_stream_insert<T>
#else
inline constexpr auto series_stream_insert<T, std::enable_if_t<is_same_cvr_v<T, ns::s1_t>>>
#endif
    = [](std::ostream &os, auto &&) { os << "Hello world, again!"; };

} // namespace obake::customisation

TEST_CASE("series_stream_insert_customization")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, ns::tag00>;

    std::ostringstream oss;

    oss << s1_t{};
    REQUIRE(boost::contains(oss.str(), "Hello world!"));

    oss.str("");
    oss << ns::s1_t{};
    REQUIRE(boost::contains(oss.str(), "Hello world, again!"));

    // Check the type returned by the streaming operator.
    REQUIRE(std::is_same_v<decltype(oss << s1_t{}), std::ostream &>);
    REQUIRE(std::is_same_v<decltype(oss << ns::s1_t{}), std::ostream &>);
}

struct foo {
};

TEST_CASE("series_add")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using s1a_t = series<pm_t, double, void>;
    using s2_t = series<pm_t, s1_t, void>;
    using s2a_t = series<pm_t, s1a_t, void>;
    using s3_t = series<pm_t, s2_t, void>;

    REQUIRE(!is_addable_v<s1_t, void>);
    REQUIRE(!is_addable_v<void, s1_t>);
    REQUIRE(!is_addable_v<s1_t, foo>);
    REQUIRE(!is_addable_v<foo, s1_t>);

    // Test rank-1 series vs scalar.
    s1_t s1;
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, "4/5");

    // Test with different scalar type,
    // coefficient will be rat_t.
    auto tmp = s1 + 2;
    REQUIRE(tmp.size() == 2u);
    for (const auto &p : tmp) {
        REQUIRE((p.second == rat_t{4, 5} || p.second == 2));
    }

    tmp = 2 + s1;
    REQUIRE(tmp.size() == 2u);
    for (const auto &p : tmp) {
        REQUIRE((p.second == rat_t{4, 5} || p.second == 2));
    }

    REQUIRE(std::is_same_v<s1_t, decltype(s1 + 2)>);
    REQUIRE(std::is_same_v<s1_t, decltype(2 + s1)>);

    // Test with same scalar type.
    tmp = s1 + rat_t{2};
    REQUIRE(tmp.size() == 2u);
    for (const auto &p : tmp) {
        REQUIRE((p.second == rat_t{4, 5} || p.second == 2));
    }

    tmp = rat_t{2} + s1;
    REQUIRE(tmp.size() == 2u);
    for (const auto &p : tmp) {
        REQUIRE((p.second == rat_t{4, 5} || p.second == 2));
    }

    REQUIRE(std::is_same_v<s1_t, decltype(s1 + rat_t{2})>);
    REQUIRE(std::is_same_v<s1_t, decltype(rat_t{2} + s1)>);

    // Test with double, will return a series
    // with double coefficient.
    auto tmp2 = s1 + 2.;
    REQUIRE(tmp2.size() == 2u);
    for (const auto &p : tmp2) {
        REQUIRE((p.second == static_cast<double>(rat_t{4, 5}) || p.second == 2.));
    }

    tmp2 = 2. + s1;
    REQUIRE(tmp2.size() == 2u);
    for (const auto &p : tmp2) {
        REQUIRE((p.second == static_cast<double>(rat_t{4, 5}) || p.second == 2.));
    }

    REQUIRE(std::is_same_v<s1a_t, decltype(s1 + 2.)>);
    REQUIRE(std::is_same_v<s1a_t, decltype(2. + s1)>);

    // Test rank-1 vs rank-2.
    s2_t s2;
    s2.set_symbol_set(symbol_set{"a", "b", "c"});
    s2.add_term(pm_t{-1, -2, -3}, "4/5");
    s2.add_term(pm_t{1, 2, 3}, s1);

    auto tmp3 = s2 + s1;
    REQUIRE(tmp3.size() == 3u);
    for (const auto &p : tmp3) {
        REQUIRE((p.second.begin()->second == rat_t{4, 5}));
    }

    tmp3 = s1 + s2;
    REQUIRE(tmp3.size() == 3u);
    for (const auto &p : tmp3) {
        REQUIRE((p.second.begin()->second == rat_t{4, 5}));
    }

    REQUIRE(std::is_same_v<s2_t, decltype(s1 + s2)>);
    REQUIRE(std::is_same_v<s2_t, decltype(s2 + s1)>);

    // Test case in which the return type is different from either
    // input type.
    s1a_t s1a;
    s1a.set_symbol_set(symbol_set{"x", "y", "z"});
    s1a.add_term(pm_t{10, 11, 12}, -3);

    auto tmp4 = s1a + s2;
    REQUIRE(tmp4.size() == 3u);
    for (const auto &p : tmp4) {
        REQUIRE((p.second.begin()->second == static_cast<double>(rat_t{4, 5}) || p.second.begin()->second == -3.));
    }

    tmp4 = s2 + s1a;
    REQUIRE(tmp4.size() == 3u);
    for (const auto &p : tmp4) {
        REQUIRE((p.second.begin()->second == static_cast<double>(rat_t{4, 5}) || p.second.begin()->second == -3.));
    }

    REQUIRE(std::is_same_v<s2a_t, decltype(s1a + s2)>);
    REQUIRE(std::is_same_v<s2a_t, decltype(s2 + s1a)>);

    // Polynomial-like test.
    s1_t x;
    x.set_symbol_set(symbol_set{"x"});
    x.add_term(pm_t{1}, 1);

    s2_t y;
    y.set_symbol_set(symbol_set{"y"});
    y.add_term(pm_t{1}, 2);

    s3_t z;
    z.set_symbol_set(symbol_set{"z"});
    z.add_term(pm_t{1}, 3);

    auto tmp5 = x + y + z;
    REQUIRE(tmp5.size() == 2u);
    for (const auto &p1 : tmp5) {
        REQUIRE((p1.second.size() == 1u || p1.second.size() == 2u));
        REQUIRE((p1.first == pm_t{0} || p1.first == pm_t{1}));
        for (const auto &p2 : p1.second) {
            REQUIRE(p2.second.size() == 1u);
            REQUIRE((p2.first == pm_t{0} || p2.first == pm_t{1}));
            for (const auto &p3 : p2.second) {
                REQUIRE((p3.second == 1 || p3.second == 2 || p3.second == 3));
            }
        }
    }

    // NOTE: this will check that round brackets in the stream output
    // are elided when the key is unitary.
    REQUIRE(!boost::contains(boost::lexical_cast<std::string>(tmp5), ")"));
    REQUIRE(!boost::contains(boost::lexical_cast<std::string>(tmp5), "("));

    REQUIRE(std::is_same_v<s3_t, decltype(x + y + z)>);
    REQUIRE(std::is_same_v<s3_t, decltype(y + z + x)>);
    REQUIRE(std::is_same_v<s3_t, decltype(y + x + z)>);
    REQUIRE(std::is_same_v<s3_t, decltype(z + y + x)>);

    // Same rank.
    // Try with various segmentations.
    for (auto s_idx1 : {0u, 1u, 2u, 4u}) {
        for (auto s_idx2 : {0u, 1u, 2u, 4u}) {
            // Test identical symbol sets.
            s1_t a;
            a.set_n_segments(s_idx1);
            a.set_symbol_set(symbol_set{"x", "y", "z"});
            a.add_term(pm_t{1, 2, 3}, "4/5");

            s1_t b;
            b.set_n_segments(s_idx2);
            b.set_symbol_set(symbol_set{"x", "y", "z"});
            b.add_term(pm_t{4, 5, 6}, "-4/5");

            auto c = a + b;
            REQUIRE(c.size() == 2u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{4, 5} || p.second == rat_t{-4, 5}));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6}));
            }

            c = b + a;
            REQUIRE(c.size() == 2u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{4, 5} || p.second == rat_t{-4, 5}));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6}));
            }

            // Add further terms to a.
            a.add_term(pm_t{-1, -2, -3}, 2);
            a.add_term(pm_t{-4, -5, -6}, -2);

            c = a + b;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }

            c = b + a;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }

            // With moves.
            auto a_copy(a);
            auto b_copy(b);

            c = std::move(a) + b;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }
            a = a_copy;

            c = a + std::move(b);
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }
            b = b_copy;

            c = std::move(a) + std::move(b);
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }

            // Test term cancellation.
            a = s1_t{};
            a.set_n_segments(s_idx1);
            a.set_symbol_set(symbol_set{"x", "y", "z"});
            a.add_term(pm_t{1, 2, 3}, "4/5");

            b = s1_t{};
            b.set_n_segments(s_idx2);
            b.set_symbol_set(symbol_set{"x", "y", "z"});
            b.add_term(pm_t{4, 5, 6}, "-4/5");
            b.add_term(pm_t{1, 2, 3}, "-4/5");

            c = a + b;
            REQUIRE(c.size() == 1u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            REQUIRE(c.begin()->second == rat_t{4, -5});
            REQUIRE(c.begin()->first == pm_t{4, 5, 6});

            c = b + a;
            REQUIRE(c.size() == 1u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            REQUIRE(c.begin()->second == rat_t{4, -5});
            REQUIRE(c.begin()->first == pm_t{4, 5, 6});

            // Test overlapping operands.
            a = a_copy;

            c = a + a;
            REQUIRE(c == 2 * a);

            // With moves as well.
            c = std::move(a) + a;
            REQUIRE(c == 2 * a_copy);
            a = a_copy;

            c = a + std::move(a);
            REQUIRE(c == 2 * a_copy);
            a = a_copy;

            // Test with heterogeneous cf types.
            a = a_copy;
            b = b_copy;
            s1a_t ax(a), bx(b);

            auto cx = ax + b;
            REQUIRE(cx.size() == 4u);
            REQUIRE(cx.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : cx) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }

            cx = bx + a;
            REQUIRE(cx.size() == 4u);
            REQUIRE(cx.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : cx) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }

            // With moves.
            auto ax_copy(ax);
            auto bx_copy(bx);

            cx = std::move(ax) + b;
            REQUIRE(cx.size() == 4u);
            REQUIRE(cx.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : cx) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }
            ax = ax_copy;

            cx = ax + std::move(b);
            REQUIRE(cx.size() == 4u);
            REQUIRE(cx.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : cx) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }
            b = b_copy;

            cx = std::move(ax) + std::move(b);
            REQUIRE(cx.size() == 4u);
            REQUIRE(cx.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : cx) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }
            ax = ax_copy;
            b = b_copy;

            // Test with different symbol sets.

            // Some random testing. Here we'll be able
            // to verify only the symbol set and the series' sizes
            // (however the asserts in the code will provide
            // more checking).
            for (auto i = 0; i < ntrials; ++i) {
                std::uniform_int_distribution<int> bdist(0, 2);

                symbol_set ss1, ss2, mss;
                for (auto j = 0; j < 6; ++j) {
                    const auto btmp = bdist(rng);
                    if (btmp == 0) {
                        ss1.insert(ss1.end(), "x" + std::to_string(j));
                    } else if (btmp == 1) {
                        ss2.insert(ss2.end(), "x" + std::to_string(j));
                    } else {
                        ss1.insert(ss1.end(), "x" + std::to_string(j));
                        ss2.insert(ss2.end(), "x" + std::to_string(j));
                    }
                    mss.insert(mss.end(), "x" + std::to_string(j));
                }

                std::uniform_int_distribution<int> edist(-3, 3), cdist(0, 10);
                std::uniform_int_distribution<unsigned> sdist(0, 6);

                a = s1_t{};
                a.set_n_segments(s_idx1);
                a.set_symbol_set(ss1);
                const auto size1 = sdist(rng);
                for (unsigned j = 0; j < size1; ++j) {
                    std::vector<int> tmp_v;
                    for (const auto &_ : ss1) {
                        detail::ignore(_);
                        tmp_v.push_back(edist(rng));
                    }
                    a.add_term(pm_t(tmp_v), cdist(rng));
                }

                b = s1_t{};
                b.set_n_segments(s_idx2);
                b.set_symbol_set(ss2);
                const auto size2 = sdist(rng);
                for (unsigned j = 0; j < size2; ++j) {
                    std::vector<int> tmp_v;
                    for (const auto &_ : ss2) {
                        detail::ignore(_);
                        tmp_v.push_back(edist(rng));
                    }
                    b.add_term(pm_t(tmp_v), cdist(rng));
                }

                c = a + b;
                REQUIRE(c.get_symbol_set() == mss);
                REQUIRE(c.size() <= size1 + size2);

                a_copy = a;
                b_copy = b;

                // Try with moves too.
                c = std::move(a) + b;
                REQUIRE(c.get_symbol_set() == mss);
                REQUIRE(c.size() <= size1 + size2);
                a = a_copy;

                c = a + std::move(b);
                REQUIRE(c.get_symbol_set() == mss);
                REQUIRE(c.size() <= size1 + size2);
                b = b_copy;

                c = std::move(a) + std::move(b);
                REQUIRE(c.get_symbol_set() == mss);
                REQUIRE(c.size() <= size1 + size2);
                a = a_copy;
                b = b_copy;
            }

            // Shorter tests in which we do more checking.
            a = s1_t{};
            a.set_n_segments(s_idx1);
            a.set_symbol_set(symbol_set{"x", "y", "z", "zz"});
            a.add_term(pm_t{1, 2, 3, -1}, "4/5");

            b = s1_t{};
            b.set_n_segments(s_idx2);
            b.set_symbol_set(symbol_set{"x", "y", "z"});
            b.add_term(pm_t{4, 5, 6}, "-4/5");

            c = a + b;
            REQUIRE(c.size() == 2u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{4, 5} || p.second == rat_t{-4, 5}));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0}));
            }

            c = b + a;
            REQUIRE(c.size() == 2u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{4, 5} || p.second == rat_t{-4, 5}));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0}));
            }

            // Add further terms to a.
            a.add_term(pm_t{-1, -2, -3, -4}, 2);
            a.add_term(pm_t{-4, -5, -6, -7}, -2);

            c = a + b;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0} || p.first == pm_t{-1, -2, -3, -4}
                         || p.first == pm_t{-4, -5, -6, -7}));
            }

            c = b + a;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0} || p.first == pm_t{-1, -2, -3, -4}
                         || p.first == pm_t{-4, -5, -6, -7}));
            }

            // With moves.
            a_copy = a;
            b_copy = b;

            c = std::move(a) + b;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0} || p.first == pm_t{-1, -2, -3, -4}
                         || p.first == pm_t{-4, -5, -6, -7}));
            }
            a = a_copy;

            c = a + std::move(b);
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0} || p.first == pm_t{-1, -2, -3, -4}
                         || p.first == pm_t{-4, -5, -6, -7}));
            }
            b = b_copy;

            c = std::move(a) + std::move(b);
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0} || p.first == pm_t{-1, -2, -3, -4}
                         || p.first == pm_t{-4, -5, -6, -7}));
            }
            a = a_copy;
            b = b_copy;
        }
    }
}

TEST_CASE("series_sub")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, void>;
    using s1a_t = series<pm_t, double, void>;
    using s2_t = series<pm_t, s1_t, void>;
    using s2a_t = series<pm_t, s1a_t, void>;
    using s3_t = series<pm_t, s2_t, void>;

    REQUIRE(!is_subtractable_v<s1_t, void>);
    REQUIRE(!is_subtractable_v<void, s1_t>);
    REQUIRE(!is_subtractable_v<s1_t, foo>);
    REQUIRE(!is_subtractable_v<foo, s1_t>);

    // Test rank-1 series vs scalar.
    s1_t s1;
    s1.set_symbol_set(symbol_set{"x", "y", "z"});
    s1.add_term(pm_t{1, 2, 3}, "4/5");

    // Test with different scalar type,
    // coefficient will be rat_t.
    auto tmp = s1 - 2;
    REQUIRE(tmp.size() == 2u);
    for (const auto &p : tmp) {
        REQUIRE((p.second == rat_t{4, 5} || p.second == -2));
    }

    tmp = 2 - s1;
    REQUIRE(tmp.size() == 2u);
    for (const auto &p : tmp) {
        REQUIRE((p.second == rat_t{-4, 5} || p.second == 2));
    }

    REQUIRE(std::is_same_v<s1_t, decltype(s1 - 2)>);
    REQUIRE(std::is_same_v<s1_t, decltype(2 - s1)>);

    // Test with same scalar type.
    tmp = s1 - rat_t{2};
    REQUIRE(tmp.size() == 2u);
    for (const auto &p : tmp) {
        REQUIRE((p.second == rat_t{4, 5} || p.second == -2));
    }

    tmp = rat_t{2} - s1;
    REQUIRE(tmp.size() == 2u);
    for (const auto &p : tmp) {
        REQUIRE((p.second == rat_t{-4, 5} || p.second == 2));
    }

    REQUIRE(std::is_same_v<s1_t, decltype(s1 - rat_t{2})>);
    REQUIRE(std::is_same_v<s1_t, decltype(rat_t{2} - s1)>);

    // Test with double, will return a series
    // with double coefficient.
    auto tmp2 = s1 - 2.;
    REQUIRE(tmp2.size() == 2u);
    for (const auto &p : tmp2) {
        REQUIRE((p.second == static_cast<double>(rat_t{4, 5}) || p.second == -2.));
    }

    tmp2 = 2. - s1;
    REQUIRE(tmp2.size() == 2u);
    for (const auto &p : tmp2) {
        REQUIRE((p.second == static_cast<double>(rat_t{-4, 5}) || p.second == 2.));
    }

    REQUIRE(std::is_same_v<s1a_t, decltype(s1 - 2.)>);
    REQUIRE(std::is_same_v<s1a_t, decltype(2. - s1)>);

    // Test rank-1 vs rank-2.
    s2_t s2;
    s2.set_symbol_set(symbol_set{"a", "b", "c"});
    s2.add_term(pm_t{-1, -2, -3}, "4/5");
    s2.add_term(pm_t{1, 2, 3}, s1);

    auto tmp3 = s2 - s1;
    REQUIRE(tmp3.size() == 3u);
    for (const auto &p : tmp3) {
        REQUIRE((abs(p.second.begin()->second) == rat_t{4, 5}));
    }

    tmp3 = s1 - s2;
    REQUIRE(tmp3.size() == 3u);
    for (const auto &p : tmp3) {
        REQUIRE((abs(p.second.begin()->second) == rat_t{4, 5}));
    }

    REQUIRE(std::is_same_v<s2_t, decltype(s1 - s2)>);
    REQUIRE(std::is_same_v<s2_t, decltype(s2 - s1)>);

    // Test case in which the return type is different from either
    // input type.
    s1a_t s1a;
    s1a.set_symbol_set(symbol_set{"x", "y", "z"});
    s1a.add_term(pm_t{10, 11, 12}, -3);

    auto tmp4 = s1a - s2;
    REQUIRE(tmp4.size() == 3u);
    for (const auto &p : tmp4) {
        REQUIRE((p.second.begin()->second == static_cast<double>(rat_t{-4, 5}) || p.second.begin()->second == -3.));
    }

    tmp4 = s2 - s1a;
    REQUIRE(tmp4.size() == 3u);
    for (const auto &p : tmp4) {
        REQUIRE((p.second.begin()->second == static_cast<double>(rat_t{4, 5}) || p.second.begin()->second == 3.));
    }

    REQUIRE(std::is_same_v<s2a_t, decltype(s1a - s2)>);
    REQUIRE(std::is_same_v<s2a_t, decltype(s2 - s1a)>);

    // Polynomial-like test.
    s1_t x;
    x.set_symbol_set(symbol_set{"x"});
    x.add_term(pm_t{1}, 1);

    s2_t y;
    y.set_symbol_set(symbol_set{"y"});
    y.add_term(pm_t{1}, 2);

    s3_t z;
    z.set_symbol_set(symbol_set{"z"});
    z.add_term(pm_t{1}, 3);

    auto tmp5 = x - y - z;
    REQUIRE(tmp5.size() == 2u);
    for (const auto &p1 : tmp5) {
        REQUIRE((p1.second.size() == 1u || p1.second.size() == 2u));
        REQUIRE((p1.first == pm_t{0} || p1.first == pm_t{1}));
        for (const auto &p2 : p1.second) {
            REQUIRE(p2.second.size() == 1u);
            REQUIRE((p2.first == pm_t{0} || p2.first == pm_t{1}));
            for (const auto &p3 : p2.second) {
                REQUIRE((p3.second == 1 || p3.second == -2 || p3.second == -3));
            }
        }
    }

    // NOTE: this will check that round brackets in the stream output
    // are elided when the key is unitary.
    REQUIRE(!boost::contains(boost::lexical_cast<std::string>(tmp5), ")"));
    REQUIRE(!boost::contains(boost::lexical_cast<std::string>(tmp5), "("));

    REQUIRE(std::is_same_v<s3_t, decltype(x - y - z)>);
    REQUIRE(std::is_same_v<s3_t, decltype(y - z - x)>);
    REQUIRE(std::is_same_v<s3_t, decltype(y - x - z)>);
    REQUIRE(std::is_same_v<s3_t, decltype(z - y - x)>);

    // Same rank.
    // Try with various segmentations.
    for (auto s_idx1 : {0u, 1u, 2u, 4u}) {
        for (auto s_idx2 : {0u, 1u, 2u, 4u}) {
            // Test identical symbol sets.
            s1_t a;
            a.set_n_segments(s_idx1);
            a.set_symbol_set(symbol_set{"x", "y", "z"});
            a.add_term(pm_t{1, 2, 3}, "4/5");

            s1_t b;
            b.set_n_segments(s_idx2);
            b.set_symbol_set(symbol_set{"x", "y", "z"});
            b.add_term(pm_t{4, 5, 6}, "-4/5");

            auto c = a - b;
            REQUIRE(c.size() == 2u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{4, 5}));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6}));
            }

            c = b - a;
            REQUIRE(c.size() == 2u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{-4, 5}));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6}));
            }

            // Add further terms to a.
            a.add_term(pm_t{-1, -2, -3}, 2);
            a.add_term(pm_t{-4, -5, -6}, -2);

            c = a - b;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }

            c = b - a;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }

            // With moves.
            auto a_copy(a);
            auto b_copy(b);

            c = std::move(a) - b;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }
            a = a_copy;

            c = a - std::move(b);
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }
            b = b_copy;

            c = std::move(a) - std::move(b);
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : c) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }

            // Test term cancellation.
            a = s1_t{};
            a.set_n_segments(s_idx1);
            a.set_symbol_set(symbol_set{"x", "y", "z"});
            a.add_term(pm_t{1, 2, 3}, "4/5");

            b = s1_t{};
            b.set_n_segments(s_idx2);
            b.set_symbol_set(symbol_set{"x", "y", "z"});
            b.add_term(pm_t{4, 5, 6}, "-4/5");
            b.add_term(pm_t{1, 2, 3}, "4/5");

            c = a - b;
            REQUIRE(c.size() == 1u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            REQUIRE(c.begin()->second == rat_t{4, 5});
            REQUIRE(c.begin()->first == pm_t{4, 5, 6});

            c = b - a;
            REQUIRE(c.size() == 1u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z"});
            REQUIRE(c.begin()->second == rat_t{4, -5});
            REQUIRE(c.begin()->first == pm_t{4, 5, 6});

            // Test overlapping operands.
            a = a_copy;

            c = a - a;
            REQUIRE(c == 0);

            // With moves as well.
            c = std::move(a) - a;
            REQUIRE(c == 0);
            a = a_copy;

            c = a - std::move(a);
            REQUIRE(c == 0);
            a = a_copy;

            // Test with heterogeneous cf types.
            a = a_copy;
            b = b_copy;
            s1a_t ax(a), bx(b);

            auto cx = ax - b;
            REQUIRE(cx.size() == 4u);
            REQUIRE(cx.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : cx) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }

            cx = bx - a;
            REQUIRE(cx.size() == 4u);
            REQUIRE(cx.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : cx) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }

            // With moves.
            auto ax_copy(ax);
            auto bx_copy(bx);

            cx = std::move(ax) - b;
            REQUIRE(cx.size() == 4u);
            REQUIRE(cx.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : cx) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }
            ax = ax_copy;

            cx = ax - std::move(b);
            REQUIRE(cx.size() == 4u);
            REQUIRE(cx.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : cx) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }
            b = b_copy;

            cx = std::move(ax) - std::move(b);
            REQUIRE(cx.size() == 4u);
            REQUIRE(cx.get_symbol_set() == symbol_set{"x", "y", "z"});
            for (const auto &p : cx) {
                REQUIRE((abs(p.second) == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3} || p.first == pm_t{4, 5, 6} || p.first == pm_t{-1, -2, -3}
                         || p.first == pm_t{-4, -5, -6}));
            }
            ax = ax_copy;
            b = b_copy;

            // Test with different symbol sets.

            // Some random testing. Here we'll be able
            // to verify only the symbol set and the series' sizes
            // (however the asserts in the code will provide
            // more checking).
            for (auto i = 0; i < ntrials; ++i) {
                std::uniform_int_distribution<int> bdist(0, 2);

                symbol_set ss1, ss2, mss;
                for (auto j = 0; j < 6; ++j) {
                    const auto btmp = bdist(rng);
                    if (btmp == 0) {
                        ss1.insert(ss1.end(), "x" + std::to_string(j));
                    } else if (btmp == 1) {
                        ss2.insert(ss2.end(), "x" + std::to_string(j));
                    } else {
                        ss1.insert(ss1.end(), "x" + std::to_string(j));
                        ss2.insert(ss2.end(), "x" + std::to_string(j));
                    }
                    mss.insert(mss.end(), "x" + std::to_string(j));
                }

                std::uniform_int_distribution<int> edist(-3, 3), cdist(0, 10);
                std::uniform_int_distribution<unsigned> sdist(0, 6);

                a = s1_t{};
                a.set_n_segments(s_idx1);
                a.set_symbol_set(ss1);
                const auto size1 = sdist(rng);
                for (unsigned j = 0; j < size1; ++j) {
                    std::vector<int> tmp_v;
                    for (const auto &_ : ss1) {
                        detail::ignore(_);
                        tmp_v.push_back(edist(rng));
                    }
                    a.add_term(pm_t(tmp_v), cdist(rng));
                }

                b = s1_t{};
                b.set_n_segments(s_idx2);
                b.set_symbol_set(ss2);
                const auto size2 = sdist(rng);
                for (unsigned j = 0; j < size2; ++j) {
                    std::vector<int> tmp_v;
                    for (const auto &_ : ss2) {
                        detail::ignore(_);
                        tmp_v.push_back(edist(rng));
                    }
                    b.add_term(pm_t(tmp_v), cdist(rng));
                }

                c = a - b;
                REQUIRE(c.get_symbol_set() == mss);
                REQUIRE(c.size() <= size1 + size2);

                a_copy = a;
                b_copy = b;

                // Try with moves too.
                c = std::move(a) - b;
                REQUIRE(c.get_symbol_set() == mss);
                REQUIRE(c.size() <= size1 + size2);
                a = a_copy;

                c = a - std::move(b);
                REQUIRE(c.get_symbol_set() == mss);
                REQUIRE(c.size() <= size1 + size2);
                b = b_copy;

                c = std::move(a) - std::move(b);
                REQUIRE(c.get_symbol_set() == mss);
                REQUIRE(c.size() <= size1 + size2);
                a = a_copy;
                b = b_copy;
            }

            // Shorter tests in which we do more checking.
            a = s1_t{};
            a.set_n_segments(s_idx1);
            a.set_symbol_set(symbol_set{"x", "y", "z", "zz"});
            a.add_term(pm_t{1, 2, 3, -1}, "4/5");

            b = s1_t{};
            b.set_n_segments(s_idx2);
            b.set_symbol_set(symbol_set{"x", "y", "z"});
            b.add_term(pm_t{4, 5, 6}, "-4/5");

            c = a - b;
            REQUIRE(c.size() == 2u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{4, 5}));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0}));
            }

            c = b - a;
            REQUIRE(c.size() == 2u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{-4, 5}));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0}));
            }

            // Add further terms to a.
            a.add_term(pm_t{-1, -2, -3, -4}, 2);
            a.add_term(pm_t{-4, -5, -6, -7}, -2);

            c = a - b;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0} || p.first == pm_t{-1, -2, -3, -4}
                         || p.first == pm_t{-4, -5, -6, -7}));
            }

            c = b - a;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{-4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0} || p.first == pm_t{-1, -2, -3, -4}
                         || p.first == pm_t{-4, -5, -6, -7}));
            }

            // With moves.
            a_copy = a;
            b_copy = b;

            c = std::move(a) - b;
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0} || p.first == pm_t{-1, -2, -3, -4}
                         || p.first == pm_t{-4, -5, -6, -7}));
            }
            a = a_copy;

            c = a - std::move(b);
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0} || p.first == pm_t{-1, -2, -3, -4}
                         || p.first == pm_t{-4, -5, -6, -7}));
            }
            b = b_copy;

            c = std::move(a) - std::move(b);
            REQUIRE(c.size() == 4u);
            REQUIRE(c.get_symbol_set() == symbol_set{"x", "y", "z", "zz"});
            for (const auto &p : c) {
                REQUIRE((p.second == rat_t{4, 5} || abs(p.second) == 2));
                REQUIRE((p.first == pm_t{1, 2, 3, -1} || p.first == pm_t{4, 5, 6, 0} || p.first == pm_t{-1, -2, -3, -4}
                         || p.first == pm_t{-4, -5, -6, -7}));
            }
            a = a_copy;
            b = b_copy;
        }
    }
}

namespace ns
{

// ADL-based customisation.
template <typename K, typename C>
inline bool series_add(const series<K, C, tag00> &, const series<K, C, tag00> &)
{
    return true;
}

} // namespace ns

struct custom_add {
    template <typename T, typename U>
    bool operator()(T &&, U &&) const
    {
        return false;
    }
};

// External customisation.
namespace obake::customisation
{

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, series<ns::pm_t, rat_t, ns::tag01>>
    &&SameCvr<U, series<ns::pm_t, rat_t, ns::tag01>> inline constexpr auto series_add<T, U>
#else
inline constexpr auto
    series_add<T, U,
               std::enable_if_t<std::conjunction_v<is_same_cvr<T, series<ns::pm_t, rat_t, ns::tag01>>,
                                                   is_same_cvr<U, series<ns::pm_t, rat_t, ns::tag01>>>>>
#endif
    = custom_add{};

} // namespace obake::customisation

TEST_CASE("series_add_custom")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, ns::tag00>;
    using s2_t = series<ns::pm_t, rat_t, ns::tag01>;

    REQUIRE(std::is_same_v<bool, decltype(s1_t{} + s1_t{})>);
    REQUIRE(s1_t{} + s1_t{} == true);

    REQUIRE(std::is_same_v<bool, decltype(s2_t{} + s2_t{})>);
    REQUIRE(s2_t{} + s2_t{} == false);
}

namespace ns
{

// ADL-based customisation.
template <typename K, typename C>
inline bool series_sub(const series<K, C, tag00> &, const series<K, C, tag00> &)
{
    return true;
}

} // namespace ns

struct custom_sub {
    template <typename T, typename U>
    bool operator()(T &&, U &&) const
    {
        return false;
    }
};

// External customisation.
namespace obake::customisation
{

template <typename T, typename U>
#if defined(OBAKE_HAVE_CONCEPTS)
requires SameCvr<T, series<ns::pm_t, rat_t, ns::tag01>>
    &&SameCvr<U, series<ns::pm_t, rat_t, ns::tag01>> inline constexpr auto series_sub<T, U>
#else
inline constexpr auto
    series_sub<T, U,
               std::enable_if_t<std::conjunction_v<is_same_cvr<T, series<ns::pm_t, rat_t, ns::tag01>>,
                                                   is_same_cvr<U, series<ns::pm_t, rat_t, ns::tag01>>>>>
#endif
    = custom_sub{};

} // namespace obake::customisation

TEST_CASE("series_sub_custom")
{
    using pm_t = packed_monomial<int>;
    using s1_t = series<pm_t, rat_t, ns::tag00>;
    using s2_t = series<ns::pm_t, rat_t, ns::tag01>;

    REQUIRE(std::is_same_v<bool, decltype(s1_t{} - s1_t{})>);
    REQUIRE(s1_t{} - s1_t{} == true);

    REQUIRE(std::is_same_v<bool, decltype(s2_t{} - s2_t{})>);
    REQUIRE(s2_t{} - s2_t{} == false);
}
