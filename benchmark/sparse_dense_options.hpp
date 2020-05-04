// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_BENCHMARK_SPARSE_DENSE_OPTIONS_HPP
#define OBAKE_BENCHMARK_SPARSE_DENSE_OPTIONS_HPP

#include <tuple>

namespace obake_benchmark
{

std::tuple<int, int> sparse_dense_options(int, char **, int);

}

#endif
