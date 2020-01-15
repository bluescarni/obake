// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_BENCHMARK_SPARSE_HPP
#define OBAKE_BENCHMARK_SPARSE_HPP

#include <iostream>

#include <obake/polynomials/polynomial.hpp>

#include "simple_timer.hpp"

namespace obake_benchmark
{

template <typename M, typename C>
inline auto sparse_benchmark(int n)
{
    using namespace obake;

    auto [x, y, z, t, u] = make_polynomials<polynomial<M, C>>("x", "y", "z", "t", "u");

    auto f = (x + y + z * z * 2 + t * t * t * 3 + u * u * u * u * u * 5 + 1);
    const auto tmp_f(f);
    auto g = (u + t + z * z * 2 + y * y * y * 3 + x * x * x * x * x * 5 + 1);
    const auto tmp_g(g);

    for (int i = 1; i < n; ++i) {
        f *= tmp_f;
        g *= tmp_g;
    }

    polynomial<M, C> ret;
    {
        simple_timer t;
        ret = f * g;
    }

    std::cout << ret.table_stats() << '\n';

    return ret;
}

} // namespace obake_benchmark

#endif
