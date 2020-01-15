// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <new>
#include <stdexcept>

#include <obake/exceptions.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

[[noreturn]] void foo()
{
    obake_throw(std::invalid_argument, "dsadsada");
}

[[noreturn]] void bar()
{
    obake_throw(std::bad_alloc, );
}

#if defined(_MSC_VER)

#pragma warning(push)
#pragma warning(disable : 4702)

#endif

TEST_CASE("exceptions_test")
{
    OBAKE_REQUIRES_THROWS_CONTAINS(foo(), std::invalid_argument, "dsadsada");
    OBAKE_REQUIRES_THROWS_CONTAINS(foo(), std::invalid_argument, "Exception type");
    OBAKE_REQUIRES_THROWS_CONTAINS(foo(), std::invalid_argument, "Exception message");
    OBAKE_REQUIRES_THROWS_CONTAINS(bar(), std::bad_alloc, "");

    // Print out an example of decorated exception.
    try {
        foo();
    } catch (const std::invalid_argument &ia) {
        std::cout << "Example of decorated exception message:\n\n\n" << ia.what() << '\n';
    }
}

#if defined(_MSC_VER)

#pragma warning(pop)

#endif
