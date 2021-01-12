// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdint>

#include <mp++/integer.hpp>

#include <obake/config.hpp>
#include <obake/polynomials/packed_monomial.hpp>

#include "dense.hpp"

using namespace obake;
using namespace obake_benchmark;

int main()
{
    dense_benchmark_5_vars<packed_monomial<
#if defined(OBAKE_PACKABLE_INT64)
                               std::uint64_t
#else
                               std::uint32_t
#endif
                               >,
                           mppp::integer<1>>(14);
}
