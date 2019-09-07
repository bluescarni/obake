// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/polynomials/packed_monomial.hpp>
#include <piranha/polynomials/polynomial.hpp>

#include <mp++/integer.hpp>

#include "simple_timer.hpp"

using namespace piranha;
using namespace piranha_benchmark;

int main()
{
    using int_t = mppp::integer<2>;
    using pm_t = packed_monomial<unsigned long>;
    using p_type = polynomial<pm_t, int_t>;

    auto [x, y, z, t, u] = make_polynomials<p_type>("x", "y", "z", "t", "u");

    auto f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
    auto tmp_f(f);
    auto g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
    auto tmp_g(g);

    for (int i = 1; i < 16; ++i) {
        f *= tmp_f;
        g *= tmp_g;
    }
    p_type ret;
    {
        simple_timer t;
        ret = f * g;
    }

    std::cout << ret.size() << '\n';
}
