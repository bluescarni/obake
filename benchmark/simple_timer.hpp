// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_BENCHMARK_SIMPLE_TIMER_HPP
#define PIRANHA_BENCHMARK_SIMPLE_TIMER_HPP

#include <chrono>
#include <iostream>

namespace piranha_benchmark
{

// A simple RAII timer class, using std::chrono. It will print, upon destruction,
// the time elapsed since construction (in ms).
class simple_timer
{
public:
    simple_timer() : m_start(std::chrono::high_resolution_clock::now()) {}
    ~simple_timer()
    {
        std::cout << "Elapsed time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()
                                                                           - m_start)
                         .count()
                  << "ms\n";
    }

private:
    const std::chrono::high_resolution_clock::time_point m_start;
};

} // namespace piranha_benchmark

#endif
