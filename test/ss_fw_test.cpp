// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>

#include <obake/symbols.hpp>

#include "catch.hpp"

std::string *get_test_address();

using namespace obake;

TEST_CASE("ss fw storage address")
{
    get_test_address()->resize(500);

    REQUIRE(get_test_address() == &detail::symbol_set_holder_class<std::string>::get());
}
