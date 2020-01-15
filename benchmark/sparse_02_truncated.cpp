// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdint>
#include <exception>
#include <iostream>

#include <mp++/integer.hpp>

#include <obake/polynomials/packed_monomial.hpp>
#include <obake/polynomials/polynomial.hpp>

#include "simple_timer.hpp"

using namespace obake;
using namespace obake_benchmark;

int main()
{
    using namespace obake;
    const auto n = 16;
    try {

        auto [x, y, z, t, u]
            = make_polynomials<polynomial<packed_monomial<std::uint64_t>, mppp::integer<2>>>("x", "y", "z", "t", "u");

        auto f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
        const auto tmp_f(f);
        auto g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
        const auto tmp_g(g);

        for (int i = 1; i < n; ++i) {
            f *= tmp_f;
            g *= tmp_g;
        }

        polynomial<packed_monomial<std::uint64_t>, mppp::integer<2>> ret;
        {
            simple_timer t;
            ret = truncated_mul(f, g, 300);
        }

        std::cout << ret.table_stats() << '\n';
    } catch (std::exception &e) {
        std::cerr << e.what();
    }
}
