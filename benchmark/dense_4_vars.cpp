// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>

#include <tbb/global_control.h>

#include <obake/polynomials/packed_monomial.hpp>

#include <mp++/integer.hpp>

#include "dense.hpp"
#include "sparse_dense_options.hpp"

using namespace obake;
using namespace obake_benchmark;

// The old benchmarks referred to these powers:
//
// - dense01 -> 30
int main(int argc, char **argv)
{
    try {
        const auto [nthreads, power] = sparse_dense_options(argc, argv, 30);

        std::optional<tbb::global_control> c;
        if (nthreads > 0) {
            c.emplace(tbb::global_control::max_allowed_parallelism, nthreads);
        }

        dense_benchmark_4_vars<packed_monomial<std::uint64_t>, mppp::integer<2>>(power);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
