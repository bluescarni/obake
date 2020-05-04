// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>

#include <boost/program_options.hpp>

#include "sparse_dense_options.hpp"

namespace po = boost::program_options;

namespace obake_benchmark
{

std::tuple<int, int> sparse_dense_options(int argc, char **argv, int default_power)
{
    int nthreads, power;

    po::options_description desc("Allowed options");

    desc.add_options()("help", "produce help message")("nthreads", po::value<int>(&nthreads)->default_value(0),
                                                       "number of threads (0 will use all cores)")(
        "power", po::value<int>(&power)->default_value(default_power), "power of the exponentiation");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        std::exit(EXIT_SUCCESS);
    }

    if (nthreads < 0) {
        throw std::invalid_argument("The number of threads must be non-negative, but it is " + std::to_string(nthreads)
                                    + " instead");
    }

    if (power < 0) {
        throw std::invalid_argument("The exponent must be non-negative, but it is " + std::to_string(power)
                                    + " instead");
    }

    return std::make_tuple(nthreads, power);
}

} // namespace obake_benchmark
