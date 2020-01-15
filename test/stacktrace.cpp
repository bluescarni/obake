// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <atomic>
#include <future>
#include <iostream>

#include <obake/stack_trace.hpp>

#include "catch.hpp"
#include "test_utils.hpp"

using namespace obake;

auto foo()
{
    return stack_trace();
}

template <int N>
auto bar(unsigned skip = 0)
{
    return bar<N - 1>(skip);
}

template <>
auto bar<0>(unsigned skip)
{
    return stack_trace(skip);
}

static std::atomic<unsigned> counter{0};

constexpr bool release_build =
#if !defined(NDEBUG)
    false
#else
    true
#endif
    ;

TEST_CASE("stack_trace")
{
    if (release_build) {
        // NOTE: don't run tests in non-debug builds.
        return;
    }
    std::cout << foo() << '\n';
    REQUIRE(!foo().empty());
    REQUIRE(bar<100>(200).empty());

    // Disable/re-enable.
    REQUIRE(stack_trace_enabled());
    set_stack_trace_enabled(false);
    REQUIRE(!stack_trace_enabled());
    REQUIRE(foo() == "<Stack trace generation has been disabled at runtime>");
    set_stack_trace_enabled(true);
    REQUIRE(stack_trace_enabled());
    REQUIRE(foo() != "<Stack trace generation has been disabled at runtime>");

    // Disable slow stack trace generation from now on.
    obake_test::disable_slow_stack_traces();

    std::cout << bar<100>() << '\n';
    REQUIRE(!bar<100>().empty());
    std::cout << bar<100>(30) << '\n';
    REQUIRE(!bar<100>(30).empty());

    // Try from different threads as well.
    // Use a barrier in order to make sure
    // all threads are running when we generate
    // the stack traces.
    auto func = []() {
        ++counter;
        while (counter.load() != 4u) {
        }
        return bar<100>();
    };
    auto fut1 = std::async(std::launch::async, func);
    auto fut2 = std::async(std::launch::async, func);
    auto fut3 = std::async(std::launch::async, func);
    auto fut4 = std::async(std::launch::async, func);

    REQUIRE(!fut1.get().empty());
    REQUIRE(!fut2.get().empty());
    REQUIRE(!fut3.get().empty());
    REQUIRE(!fut4.get().empty());
}
