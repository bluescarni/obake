// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/utils/stack_trace.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace piranha;

auto foo()
{
    return stack_trace();
}

template <int N>
auto bar()
{
    return bar<N - 1>();
}

template <>
auto bar<0>()
{
    return stack_trace();
}

TEST_CASE("utils_stack_trace")
{
    std::cout << foo() << '\n';
    std::cout << bar<100>() << '\n';
}
