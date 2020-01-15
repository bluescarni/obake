// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <thread>
#include <type_traits>
#include <vector>

#include <obake/detail/atomic_flag_array.hpp>
#include <obake/detail/atomic_lock_guard.hpp>

#include "catch.hpp"

using namespace obake;

using a_array = detail::atomic_flag_array;
using alg = detail::atomic_lock_guard;

TEST_CASE("atomic_flag_array")
{
    // Test with just an empty array.
    a_array a0(0u);
    // Non-empty.
    std::size_t size = 100u;
    a_array a1(size);
    // Verify everything is set to false.
    for (std::size_t i = 0u; i < size; ++i) {
        REQUIRE(!a1[i].test_and_set());
        REQUIRE(a1[i].test_and_set());
    }
    // Concurrent.
    size = 1000000u;
    a_array a2(size);
    std::atomic<int> tb(0);
    auto func = [&a2, &tb, size]() {
        ++tb;
        while (tb.load() != 2) {
        }
        for (std::size_t i = 0u; i < size; ++i) {
            a2[i].test_and_set();
        }
    };
    std::thread t0(func);
    std::thread t1(func);
    t0.join();
    t1.join();
    for (std::size_t i = 0u; i < size; ++i) {
        REQUIRE(a2[i].test_and_set());
        // Check also the const getter of the array.
        REQUIRE(std::addressof(a2[i]) == std::addressof(static_cast<const a_array &>(a2)[i]));
    }
    // Some type traits checks.
    REQUIRE(!std::is_constructible<a_array>::value);
    REQUIRE(!std::is_copy_constructible<a_array>::value);
    REQUIRE(!std::is_move_constructible<a_array>::value);
    REQUIRE(!std::is_copy_assignable<a_array>::value);
    REQUIRE(!std::is_move_assignable<a_array>::value);
}

TEST_CASE("atomic_lock_guard")
{
    // Some type traits checks.
    REQUIRE(!std::is_constructible<alg>::value);
    REQUIRE(!std::is_copy_constructible<alg>::value);
    REQUIRE(!std::is_move_constructible<alg>::value);
    REQUIRE(!std::is_copy_assignable<alg>::value);
    REQUIRE(!std::is_move_assignable<alg>::value);
    // Concurrent writes protected by a spinlock.
    std::size_t size = 10000u;
    using size_type = std::vector<double>::size_type;
    std::vector<double> v(size, 0.);
    a_array a0(size);
    std::atomic<int> tb(0);
    auto func = [&a0, &tb, &v, size]() {
        ++tb;
        while (tb.load() != 2) {
        }
        for (std::size_t i = 0u; i < size; ++i) {
            alg l(a0[i]);
            v[static_cast<size_type>(i)] = 1.;
        }
    };
    std::thread t0(func);
    std::thread t1(func);
    t0.join();
    t1.join();
    REQUIRE(std::all_of(v.begin(), v.end(), [](double x) { return x == 1.; }));
}
