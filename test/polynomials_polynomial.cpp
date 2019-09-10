// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/config.hpp>

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

#include "catch.hpp"
#include "test_utils.hpp"

using namespace piranha;

TEST_CASE("make_polynomials_test")
{
    using Catch::Matchers::Contains;

    using poly_t = polynomial<packed_monomial<long>, double>;

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
    using poly_t = polynomial<packed_monomial<long>, double>;

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
    using p1_t = polynomial<packed_monomial<long>, double>;
    using p2_t = polynomial<packed_monomial<int>, double>;
    using p3_t = polynomial<packed_monomial<long>, float>;

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

    using pm_t = packed_monomial<long>;

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
        a.add_term(pm_t{std::get<1>(detail::limits_minmax<long>)}, 1);
        b.add_term(pm_t{std::get<1>(detail::limits_minmax<long>)}, 1);

        std::cout << "ZE TESSSSSSST\n";

        REQUIRE_THROWS_WITH(
            polynomials::detail::poly_mul_impl_simple(retval, a, b),
            Contains(
                "An overflow in the monomial exponents was detected while attempting to multiply two polynomials"));
        REQUIRE_THROWS_AS(polynomials::detail::poly_mul_impl_simple(retval, a, b), std::overflow_error);

        a.clear();
        a.set_symbol_set(symbol_set{"a"});
        b.clear();
        b.set_symbol_set(symbol_set{"a"});
        a.add_term(pm_t{std::get<0>(detail::limits_minmax<long>)}, 1);
        b.add_term(pm_t{std::get<0>(detail::limits_minmax<long>)}, 1);

        REQUIRE_THROWS_WITH(
            polynomials::detail::poly_mul_impl_simple(retval, a, b),
            Contains(
                "An overflow in the monomial exponents was detected while attempting to multiply two polynomials"));
        REQUIRE_THROWS_AS(polynomials::detail::poly_mul_impl_simple(retval, a, b), std::overflow_error);
    });
}
