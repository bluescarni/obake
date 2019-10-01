// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_BENCHMARK_DENSE_HPP
#define OBAKE_BENCHMARK_DENSE_HPP

#include <iostream>

#include <obake/byte_size.hpp>
#include <obake/polynomials/polynomial.hpp>

#include "simple_timer.hpp"

namespace obake_benchmark
{

template <typename M, typename C>
inline auto dense_benchmark(int n)
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

    std::cout << "Total number of terms             : " << ret.size() << '\n';
    std::cout << "Total number of tables            : " << ret._get_s_table().size() << '\n';
    std::cout << "Number of terms in the first table: " << ret._get_s_table()[0].size() << '\n';
    std::cout << "Total size in bytes               : " << byte_size(ret) << '\n';

    return ret;
}

} // namespace obake_benchmark

#endif
