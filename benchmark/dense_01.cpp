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

    auto [x, y, z, t] = make_polynomials<p_type>("x", "y", "z", "t");

    auto f = x + y + z + t + 1;
    auto tmp(f);
    for (auto i = 1; i < 30; ++i) {
        f *= tmp;
    }
    p_type ret;
    {
        simple_timer t;
        ret = f * (f + 1);
    }

    std::cout << ret.size() << '\n';
}
