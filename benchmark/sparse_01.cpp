// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/polynomials/packed_monomial.hpp>

#include <mp++/integer.hpp>

#include "sparse.hpp"

using namespace piranha;
using namespace piranha_benchmark;

int main()
{
    sparse_benchmark<packed_monomial<unsigned long>, mppp::integer<2>>(12);
}
