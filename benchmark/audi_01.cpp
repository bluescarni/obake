// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>

#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>

#include "simple_timer.hpp"

using namespace obake;
using namespace obake_benchmark;

// A performance test for truncated polynomial multiplication, in the spirit of automatic differentiation.
// Compute
//
// (1+a1+a2+a3+a4+a5+a6+a7+a8+a9+a10)**10 * (1-a1-a2-a3-a4-a5-a6-a7-a8-a9-a10)**10
//
// where
//
// a_i = 1 + x_i
//
// truncated to the total degree of 10.

// Small helper to compute the truncated power to the n.
template <typename T>
auto truncated_pow(const T &x, unsigned n, unsigned limit)
{
    T retval(1);

    for (auto i = 0u; i < n; ++i) {
        retval = truncated_mul(retval, x, limit);
    }

    return retval;
}

int main()
{
    using p_type = polynomial<packed_monomial<unsigned long long>, double>;

    auto polys = make_polynomials<p_type>("x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10");

    for (auto &p : polys) {
        p += 1;
    }

    const auto &[a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = polys;

    auto f = truncated_pow(1 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10, 10, 10);
    auto g = truncated_pow(1 - a1 - a2 - a3 - a4 - a5 - a6 - a7 - a8 - a9 - a10, 10, 10);

    p_type h;
    {
        simple_timer t;
        h = truncated_mul(f, g, 10);
    }

    std::cout << h.table_stats() << '\n';

    return 0;
}
