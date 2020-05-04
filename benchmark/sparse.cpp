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
#include <stdexcept>
#include <string>

#include <boost/program_options.hpp>

#include <tbb/global_control.h>

#include <mp++/integer.hpp>

#include <obake/polynomials/packed_monomial.hpp>

#include "sparse.hpp"

namespace po = boost::program_options;

using namespace obake;
using namespace obake_benchmark;

// The old benchmarks referred to these powers:
//
// - sparse01 -> 12
// - sparse02 -> 16
// - sparse03 -> 20
// - sparse04 -> 25
int main(int argc, char **argv)
{
    int nthreads, power;

    po::options_description desc("Allowed options");

    desc.add_options()("help", "produce help message")("nthreads", po::value<int>(&nthreads)->default_value(0),
                                                       "number of threads")(
        "power", po::value<int>(&power)->default_value(12), "power of the exponentiation");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return EXIT_SUCCESS;
    }

    try {
        if (nthreads < 0) {
            throw std::invalid_argument("The number of threads must be non-negative, but it is "
                                        + std::to_string(nthreads) + " instead");
        }

        if (power < 0) {
            throw std::invalid_argument("The exponent must be non-negative, but it is " + std::to_string(power)
                                        + " instead");
        }

        std::optional<tbb::global_control> c;
        if (nthreads > 0) {
            c.emplace(tbb::global_control::max_allowed_parallelism, nthreads);
        }

        sparse_benchmark<packed_monomial<std::uint64_t>, mppp::integer<2>>(power);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
