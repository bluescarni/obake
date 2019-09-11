// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/config.hpp>

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#if defined(PIRANHA_HAVE_STRING_VIEW)

#include <string_view>

#endif

#include <mp++/integer.hpp>

#include <piranha/detail/limits.hpp>
#include <piranha/detail/tuple_for_each.hpp>
#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/polynomials/polynomial.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace piranha;

TEST_CASE("make_polynomials_test")
{
    using Catch::Matchers::Contains;

    using poly_t = polynomial<packed_monomial<long long>, double>;

    piranha_test::disable_slow_stack_traces();

    REQUIRE(make_polynomials<poly_t>().size() == 0u);
    REQUIRE(make_polynomials<poly_t>(symbol_set{}).size() == 0u);

    {
        auto [a] = make_polynomials<poly_t>("a");
        REQUIRE(a.get_symbol_set() == symbol_set{"a"});

        auto [b] = make_polynomials<poly_t>(std::string("b"));
        REQUIRE(b.get_symbol_set() == symbol_set{"b"});
    }

#if defined(PIRANHA_HAVE_STRING_VIEW)
    {
        auto [a] = make_polynomials<poly_t>(std::string_view{"a"});
        REQUIRE(a.get_symbol_set() == symbol_set{"a"});
    }
#endif

    {
        auto [a1] = make_polynomials<poly_t>(symbol_set{"a"}, "a");
        REQUIRE(a1.get_symbol_set() == symbol_set{"a"});

        auto [a2] = make_polynomials<poly_t>(symbol_set{"a", "b", "c"}, "a");
        REQUIRE(a2.get_symbol_set() == symbol_set{"a", "b", "c"});

        auto [b, c] = make_polynomials<poly_t>(symbol_set{"a", "b", "c"}, "b", std::string("c"));
        REQUIRE(b.get_symbol_set() == symbol_set{"a", "b", "c"});
        REQUIRE(c.get_symbol_set() == symbol_set{"a", "b", "c"});
    }

#if defined(PIRANHA_HAVE_STRING_VIEW)
    {
        auto [a1] = make_polynomials<poly_t>(symbol_set{"a"}, std::string_view{"a"});
        REQUIRE(a1.get_symbol_set() == symbol_set{"a"});

        auto [b, c] = make_polynomials<poly_t>(symbol_set{"a", "b", "c"}, std::string_view{"b"}, std::string("c"));
        REQUIRE(b.get_symbol_set() == symbol_set{"a", "b", "c"});
        REQUIRE(c.get_symbol_set() == symbol_set{"a", "b", "c"});
    }
#endif

    REQUIRE_THROWS_WITH(make_polynomials<poly_t>(symbol_set{"b"}, "a"),
                        Contains("Cannot create a polynomial with symbol set {'b'} from the generator 'a': the "
                                 "generator is not in the symbol set"));
    REQUIRE_THROWS_WITH(make_polynomials<poly_t>(symbol_set{}, "ada"),
                        Contains("Cannot create a polynomial with symbol set {} from the generator 'ada': the "
                                 "generator is not in the symbol set"));
    REQUIRE_THROWS_AS(make_polynomials<poly_t>(symbol_set{"b"}, "a"), std::invalid_argument);
    REQUIRE_THROWS_AS(make_polynomials<poly_t>(symbol_set{}, "ada"), std::invalid_argument);
}

TEST_CASE("is_polynomial_test")
{
    using poly_t = polynomial<packed_monomial<long long>, double>;

    REQUIRE(is_polynomial_v<poly_t>);
    REQUIRE(!is_polynomial_v<void>);
    REQUIRE(!is_polynomial_v<int>);
    REQUIRE(!is_polynomial_v<double>);
    REQUIRE(!is_polynomial_v<const poly_t &>);
    REQUIRE(!is_polynomial_v<poly_t &>);
    REQUIRE(!is_polynomial_v<poly_t &&>);
    REQUIRE(!is_polynomial_v<const poly_t>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(Polynomial<poly_t>);
    REQUIRE(!Polynomial<void>);
    REQUIRE(!Polynomial<int>);
    REQUIRE(!Polynomial<double>);
    REQUIRE(!Polynomial<const poly_t &>);
    REQUIRE(!Polynomial<poly_t &>);
    REQUIRE(!Polynomial<poly_t &&>);
    REQUIRE(!Polynomial<const poly_t>);
#endif
}

TEST_CASE("polynomial_mul_detail_test")
{
    using p1_t = polynomial<packed_monomial<long long>, double>;
    using p2_t = polynomial<packed_monomial<int>, double>;
    using p3_t = polynomial<packed_monomial<long long>, float>;

    REQUIRE(polynomials::detail::poly_mul_algo<void, void> == 0);
    REQUIRE(std::is_same_v<void, polynomials::detail::poly_mul_ret_t<void, void>>);

    REQUIRE(polynomials::detail::poly_mul_algo<p1_t, p2_t> == 0);
    REQUIRE(polynomials::detail::poly_mul_algo<p2_t, p1_t> == 0);
    REQUIRE(std::is_same_v<void, polynomials::detail::poly_mul_ret_t<p1_t, p2_t>>);
    REQUIRE(std::is_same_v<void, polynomials::detail::poly_mul_ret_t<p2_t, p1_t>>);

    REQUIRE(polynomials::detail::poly_mul_algo<p1_t, p3_t> == 1);
    REQUIRE(polynomials::detail::poly_mul_algo<p3_t, p1_t> == 1);
    REQUIRE(std::is_same_v<p1_t, polynomials::detail::poly_mul_ret_t<p1_t, p3_t>>);
    REQUIRE(std::is_same_v<p1_t, polynomials::detail::poly_mul_ret_t<p3_t, p1_t>>);
}

TEST_CASE("polynomial_mul_simpl_test")
{
    using Catch::Matchers::Contains;

    using pm_t = packed_monomial<long long>;

    using cf_types = std::tuple<double, mppp::integer<1>>;

    detail::tuple_for_each(cf_types{}, [](auto x) {
        using poly_t = polynomial<pm_t, decltype(x)>;

        // A few simple tests.
        poly_t retval;
        polynomials::detail::poly_mul_impl_simple(retval, poly_t{3}, poly_t{4});
        REQUIRE(retval == 12);
        retval.clear();

        // Examples with cancellations.
        auto [a, b, c] = make_polynomials<poly_t>(symbol_set{"a", "b", "c"}, "a", "b", "c");
        retval.set_symbol_set(symbol_set{"a", "b", "c"});
        polynomials::detail::poly_mul_impl_simple(retval, a + b, a - b);
        REQUIRE(retval == a * a - b * b);
        retval.clear();

        retval.set_symbol_set(symbol_set{"a", "b", "c"});
        polynomials::detail::poly_mul_impl_simple(retval, a * a + b * b, (a + b) * (a - b));
        REQUIRE(retval == a * a * a * a - b * b * b * b);
        retval.clear();

        // An overflowing example.
        retval.set_symbol_set(symbol_set{"a"});
        a.clear();
        a.set_symbol_set(symbol_set{"a"});
        b.clear();
        b.set_symbol_set(symbol_set{"a"});
        a.add_term(pm_t{detail::limits_max<long long>}, 1);
        b.add_term(pm_t{detail::limits_max<long long>}, 1);

        REQUIRE_THROWS_WITH(
            polynomials::detail::poly_mul_impl_simple(retval, a, b),
            Contains(
                "An overflow in the monomial exponents was detected while attempting to multiply two polynomials"));
        REQUIRE_THROWS_AS(polynomials::detail::poly_mul_impl_simple(retval, a, b), std::overflow_error);

        a.clear();
        a.set_symbol_set(symbol_set{"a"});
        b.clear();
        b.set_symbol_set(symbol_set{"a"});
        a.add_term(pm_t{detail::limits_min<long long>}, 1);
        b.add_term(pm_t{detail::limits_min<long long>}, 1);

        REQUIRE_THROWS_WITH(
            polynomials::detail::poly_mul_impl_simple(retval, a, b),
            Contains(
                "An overflow in the monomial exponents was detected while attempting to multiply two polynomials"));
        REQUIRE_THROWS_AS(polynomials::detail::poly_mul_impl_simple(retval, a, b), std::overflow_error);
    });
}

TEST_CASE("polynomial_mul_hm_mt_test")
{
    using Catch::Matchers::Contains;

    using pm_t = packed_monomial<long long>;

    using cf_types = std::tuple<double, mppp::integer<1>>;

    detail::tuple_for_each(cf_types{}, [](auto xs) {
        using poly_t = polynomial<pm_t, decltype(xs)>;

        // A few simple tests.
        poly_t retval;
        polynomials::detail::poly_mul_impl_mt_hm(retval, poly_t{3}, poly_t{4});
        REQUIRE(retval == 12);
        retval.clear();

        // Examples with cancellations.
        auto [a, b, c] = make_polynomials<poly_t>(symbol_set{"a", "b", "c"}, "a", "b", "c");
        retval.set_symbol_set(symbol_set{"a", "b", "c"});
        polynomials::detail::poly_mul_impl_mt_hm(retval, a + b, a - b);
        REQUIRE(retval == a * a - b * b);
        retval.clear();

        retval.set_symbol_set(symbol_set{"a", "b", "c"});
        polynomials::detail::poly_mul_impl_mt_hm(retval, a * a + b * b, (a + b) * (a - b));
        REQUIRE(retval == a * a * a * a - b * b * b * b);
        retval.clear();

        // An overflowing example.
        retval.set_symbol_set(symbol_set{"a"});
        a.clear();
        a.set_symbol_set(symbol_set{"a"});
        b.clear();
        b.set_symbol_set(symbol_set{"a"});
        a.add_term(pm_t{detail::limits_max<long long>}, 1);
        b.add_term(pm_t{detail::limits_max<long long>}, 1);

        REQUIRE_THROWS_WITH(
            polynomials::detail::poly_mul_impl_mt_hm(retval, a, b),
            Contains(
                "An overflow in the monomial exponents was detected while attempting to multiply two polynomials"));
        REQUIRE_THROWS_AS(polynomials::detail::poly_mul_impl_mt_hm(retval, a, b), std::overflow_error);

        a.clear();
        a.set_symbol_set(symbol_set{"a"});
        b.clear();
        b.set_symbol_set(symbol_set{"a"});
        a.add_term(pm_t{detail::limits_min<long long>}, 1);
        b.add_term(pm_t{detail::limits_min<long long>}, 1);

        REQUIRE_THROWS_WITH(
            polynomials::detail::poly_mul_impl_mt_hm(retval, a, b),
            Contains(
                "An overflow in the monomial exponents was detected while attempting to multiply two polynomials"));
        REQUIRE_THROWS_AS(polynomials::detail::poly_mul_impl_mt_hm(retval, a, b), std::overflow_error);
    });
}

TEST_CASE("polynomial_mul_larger_hm_mt_test")
{
    using Catch::Matchers::Contains;

    using pm_t = packed_monomial<long long>;

    using cf_types = std::tuple<double, mppp::integer<1>>;

    detail::tuple_for_each(cf_types{}, [](auto xs) {
        using poly_t = polynomial<pm_t, decltype(xs)>;

        // Try with larger operands.
        auto [x, y, z, t, u] = make_polynomials<poly_t>("x", "y", "z", "t", "u");

        auto f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
        const auto tmp_f(f);
        auto g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
        const auto tmp_g(g);

        for (int i = 1; i < 10; ++i) {
            f *= tmp_f;
            g *= tmp_g;
        }

        auto ret = f * g;

        REQUIRE(ret.size() == 2096600ull);
    });
}

TEST_CASE("polynomial_mul_general_test")
{
    // General test cases.
    using pm_t = packed_monomial<long long>;
    using p1_t = polynomial<pm_t, mppp::integer<1>>;
    using p2_t = polynomial<pm_t, double>;
    using p3_t = polynomial<packed_monomial<long>, mppp::integer<1>>;

    REQUIRE(is_multipliable_v<p1_t, p1_t>);
    REQUIRE(is_multipliable_v<p1_t, p2_t>);
    REQUIRE(is_multipliable_v<p2_t, p1_t>);
    REQUIRE(!is_multipliable_v<p1_t, p3_t>);
    REQUIRE(!is_multipliable_v<p3_t, p1_t>);

    REQUIRE(std::is_same_v<p2_t, decltype(p1_t{} * p2_t{})>);
    REQUIRE(std::is_same_v<p2_t, decltype(p2_t{} * p1_t{})>);
    REQUIRE(std::is_same_v<p1_t, decltype(p1_t{} * p1_t{})>);
    REQUIRE(std::is_same_v<p2_t, decltype(p2_t{} * p2_t{})>);

    {
        // Some tests with empty series.
        p1_t x1, y1;
        x1.set_symbol_set(symbol_set{"x", "y"});
        y1.set_symbol_set(symbol_set{"x", "y"});

        auto ret1 = x1 * y1;
        REQUIRE(ret1.empty());
        REQUIRE(ret1.get_symbol_set() == symbol_set{"x", "y"});

        ret1 = y1 * x1;
        REQUIRE(ret1.empty());
        REQUIRE(ret1.get_symbol_set() == symbol_set{"x", "y"});

        x1.set_symbol_set(symbol_set{"x"});

        ret1 = x1 * y1;
        REQUIRE(ret1.empty());
        REQUIRE(ret1.get_symbol_set() == symbol_set{"x", "y"});

        ret1 = y1 * x1;
        REQUIRE(ret1.empty());
        REQUIRE(ret1.get_symbol_set() == symbol_set{"x", "y"});
    }

    {
        // Test the symbol merging machinery.
        auto [x, y, z] = make_polynomials<p1_t>("x", "y", "z");

        p1_t cmp;

        // Try with both operands not needing any symbol merging.
        auto ret = (x - y) * (x + y);
        REQUIRE(ret.get_symbol_set() == symbol_set{"x", "y"});
        cmp.set_symbol_set(symbol_set{"x", "y"});
        cmp.add_term(pm_t{2, 0}, 1);
        cmp.add_term(pm_t{0, 2}, -1);
        REQUIRE(ret == cmp);

        // Try with an operand which needs to be extended.
        ret = x * (x + y);
        REQUIRE(ret.get_symbol_set() == symbol_set{"x", "y"});
        cmp.clear();
        cmp.set_symbol_set(symbol_set{"x", "y"});
        cmp.add_term(pm_t{2, 0}, 1);
        cmp.add_term(pm_t{1, 1}, 1);
        REQUIRE(ret == cmp);

        // Try with the other operand.
        ret = y * (x + y);
        REQUIRE(ret.get_symbol_set() == symbol_set{"x", "y"});
        cmp.clear();
        cmp.set_symbol_set(symbol_set{"x", "y"});
        cmp.add_term(pm_t{1, 1}, 1);
        cmp.add_term(pm_t{0, 2}, 1);
        REQUIRE(ret == cmp);

        // An example in which both operands have to be extended.
        ret = z * (x + y);
        REQUIRE(ret.get_symbol_set() == symbol_set{"x", "y", "z"});
        cmp.clear();
        cmp.set_symbol_set(symbol_set{"x", "y", "z"});
        cmp.add_term(pm_t{1, 0, 1}, 1);
        cmp.add_term(pm_t{0, 1, 1}, 1);
        REQUIRE(ret == cmp);
    }
}
