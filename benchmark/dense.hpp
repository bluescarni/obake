// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_BENCHMARK_DENSE_HPP
#define OBAKE_BENCHMARK_DENSE_HPP

#include <iostream>

#include <obake/polynomials/polynomial.hpp>

#include "simple_timer.hpp"

namespace obake_benchmark
{

template <typename M, typename C>
inline auto dense_benchmark_4_vars(int n)
{
    using namespace obake;

    auto [x, y, z, t] = make_polynomials<polynomial<M, C>>("x", "y", "z", "t");

    auto f = x + y + z + t + 1;
    const auto tmp(f);
    for (auto i = 1; i < n; ++i) {
        f *= tmp;
    }
    auto g = f + 1;

    polynomial<M, C> ret;
    {
        simple_timer t;
        ret = f * g;
    }

    std::cout << ret.table_stats() << '\n';

    return ret;
}

template <typename M, typename C>
inline auto dense_benchmark_5_vars(int n)
{
    using namespace obake;

    auto [x, y, z, t, u] = make_polynomials<polynomial<M, C>>("x", "y", "z", "t", "u");

    auto f = x + y + z + t + u + 1;
    const auto tmp(f);
    for (auto i = 1; i < n; ++i) {
        f *= tmp;
    }
    auto g = f + 1;

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
