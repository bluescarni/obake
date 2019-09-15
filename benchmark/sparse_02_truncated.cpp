// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>

#include <mp++/integer.hpp>

#include <piranha/byte_size.hpp>
#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/polynomials/polynomial.hpp>

#include "simple_timer.hpp"

using namespace piranha;
using namespace piranha_benchmark;

int main()
{
    using namespace piranha;
    const auto n = 16;

    auto [x, y, z, t, u]
        = make_polynomials<polynomial<packed_monomial<unsigned long>, mppp::integer<2>>>("x", "y", "z", "t", "u");

    auto f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
    const auto tmp_f(f);
    auto g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
    const auto tmp_g(g);

    for (int i = 1; i < n; ++i) {
        f *= tmp_f;
        g *= tmp_g;
    }

    polynomial<packed_monomial<unsigned long>, mppp::integer<2>> ret;
    {
        simple_timer t;
        ret = truncated_mul(f, g, 300);
    }

    std::cout << "Total number of terms             : " << ret.size() << '\n';
    std::cout << "Total number of tables            : " << ret._get_s_table().size() << '\n';
    std::cout << "Number of terms in the first table: " << ret._get_s_table()[0].size() << '\n';
    std::cout << "Total size in bytes               : " << byte_size(ret) << '\n';
}
