// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

// TODO temporary.
#include <iostream>

#include <mp++/integer.hpp>

#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/polynomials/polynomial.hpp>
#include <piranha/symbols.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace piranha;

TEST_CASE("polynomial_truncated_mul_test")
{
    piranha_test::disable_slow_stack_traces();

    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z] = make_polynomials<poly_t>(symbol_set{"x", "y", "z"}, "x", "y", "z");

    std::cout << truncated_mul(x * x + 4 * z + 2, y, 1) << '\n';
    std::cout << truncated_mul(x * x + 4 * z + 2, y, 2) << '\n';
    std::cout << truncated_mul(x * x + 4 * z + 2, y, 3) << '\n';
}

TEST_CASE("polynomial_mul_hm_mt_test_truncated")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z] = make_polynomials<poly_t>(symbol_set{"x", "y", "z"}, "x", "y", "z");
    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z"});

    polynomials::detail::poly_mul_impl_mt_hm(retval, x * x + 4 * z + 2, y, 1);
    std::cout << retval << '\n';

    retval.clear();
    retval.set_symbol_set(symbol_set{"x", "y", "z"});
    polynomials::detail::poly_mul_impl_mt_hm(retval, x * x + 4 * z + 2, y, 2);
    std::cout << retval << '\n';

    retval.clear();
    retval.set_symbol_set(symbol_set{"x", "y", "z"});
    polynomials::detail::poly_mul_impl_mt_hm(retval, x * x + 4 * z + 2, y, 3);
    std::cout << retval << '\n';
}

TEST_CASE("polynomial_mul_simple_test_p_truncated")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z] = make_polynomials<poly_t>(symbol_set{"x", "y", "z"}, "x", "y", "z");
    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z"});

    polynomials::detail::poly_mul_impl_simple(retval, x * x + 4 * z + 2, y, 1, symbol_set{"z"});
    std::cout << retval << '\n';

    retval.clear();
    retval.set_symbol_set(symbol_set{"x", "y", "z"});
    polynomials::detail::poly_mul_impl_simple(retval, x * x + 4 * z + 2, y, 2, symbol_set{"x", "y"});
    std::cout << retval << '\n';

    retval.clear();
    retval.set_symbol_set(symbol_set{"x", "y", "z"});
    polynomials::detail::poly_mul_impl_simple(retval, x * x + 4 * z + 2, y, 3, symbol_set{"x", "z"});
    std::cout << retval << '\n';
}

TEST_CASE("polynomial_mul_hm_mt_test_p_truncated")
{
    using pm_t = packed_monomial<long long>;
    using poly_t = polynomial<pm_t, mppp::integer<1>>;

    auto [x, y, z] = make_polynomials<poly_t>(symbol_set{"x", "y", "z"}, "x", "y", "z");
    poly_t retval;
    retval.set_symbol_set(symbol_set{"x", "y", "z"});

    polynomials::detail::poly_mul_impl_mt_hm(retval, x * x + 4 * z + 2, y, 1, symbol_set{"z"});
    std::cout << retval << '\n';

    retval.clear();
    retval.set_symbol_set(symbol_set{"x", "y", "z"});
    polynomials::detail::poly_mul_impl_mt_hm(retval, x * x + 4 * z + 2, y, 2, symbol_set{"x", "y"});
    std::cout << retval << '\n';

    retval.clear();
    retval.set_symbol_set(symbol_set{"x", "y", "z"});
    polynomials::detail::poly_mul_impl_mt_hm(retval, x * x + 4 * z + 2, y, 3, symbol_set{"x", "z"});
    std::cout << retval << '\n';
}
