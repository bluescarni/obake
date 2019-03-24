// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/exceptions.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <new>
#include <stdexcept>

[[noreturn]] void foo()
{
    piranha_throw(std::invalid_argument, "dsadsada");
}

[[noreturn]] void bar()
{
    piranha_throw(std::bad_alloc, );
}

#if defined(_MSC_VER)

#pragma warning(push)
#pragma warning(disable : 4702)

#endif

TEST_CASE("exceptions_test")
{
    using Catch::Matchers::Contains;
    REQUIRE_THROWS_WITH(foo(), Contains("dsadsada"));
    REQUIRE_THROWS_WITH(foo(), Contains("Exception type"));
    REQUIRE_THROWS_WITH(foo(), Contains("Exception message"));
    REQUIRE_THROWS_AS(bar(), std::bad_alloc);

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
