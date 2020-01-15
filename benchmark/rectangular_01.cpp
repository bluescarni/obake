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

// Test taken from:
// http://groups.google.com/group/sage-devel/browse_thread/thread/f5b976c979a3b784/1263afcc6f9d09da
// Meant to test sparse multiplication where series have very different sizes.
int main()
{
    using p_type = polynomial<packed_monomial<unsigned long long>, double>;

    auto [x, y, z] = make_polynomials<p_type>("x", "y", "z");

    auto f = x * y * y * y * z * z + x * x * y * y * z + x * y * y * y * z + x * y * y * z * z + y * y * y * z * z
             + y * y * y * z + 2 * y * y * z * z + 2 * x * y * z + y * y * z + y * z * z + y * y + 2 * y * z + z;

    p_type curr(1);

    {
        simple_timer t;
        for (auto i = 1; i <= 70; ++i) {
            curr *= f;
        }
    }

    std::cout << curr.table_stats() << '\n';

    return 0;
}
