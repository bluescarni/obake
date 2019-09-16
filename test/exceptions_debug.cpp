// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <stdexcept>

#include <piranha/exceptions.hpp>

[[noreturn]] void foo()
{
    piranha_throw(std::invalid_argument, "dsadsada");
}

#if defined(_MSC_VER)

#pragma warning(push)
#pragma warning(disable : 4702)

#endif

int main()
{
    try {
        foo();
    } catch (const std::invalid_argument &ex) {
        std::cout << "Caught: " << ex.what() << '\n';
    }
}

#if defined(_MSC_VER)

#pragma warning(pop)

#endif
