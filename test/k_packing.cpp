// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>

#include <piranha/k_packing.hpp>

#include "catch.hpp"

using namespace piranha;

TEST_CASE("k_packer_unsigned")
{
    const auto &[table, cvs, minmaxes] = detail::k_packing_unsigned_data<long long>;

    for (const auto &r : table) {
        for (const auto &n : r) {
            std::cout << n << " ";
        }
        std::cout << '\n';
    }

    std::cout << '\n';

    for (const auto &r : cvs) {
        for (const auto &n : r) {
            std::cout << n << " ";
        }
        std::cout << '\n';
    }

    std::cout << '\n';

    for (const auto &r : minmaxes) {
        for (const auto &p : r) {
            std::cout << "[" << p.first << ", " << p.second << "]"
                      << " ";
        }
        std::cout << '\n';
    }
}