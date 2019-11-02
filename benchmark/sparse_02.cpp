// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <obake/polynomials/packed_monomial.hpp>

#include <mp++/integer.hpp>

#include "sparse.hpp"

using namespace obake;
using namespace obake_benchmark;

int main()
{
    try {
        sparse_benchmark<packed_monomial<uint64_t>, mppp::integer<2>>(16);
    } catch (std::exception &e) {
        std::cerr << e.what();    
    }
}
