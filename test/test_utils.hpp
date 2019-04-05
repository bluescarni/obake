// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_TEST_UTILS_HPP
#define PIRANHA_TEST_UTILS_HPP

#include <piranha/config.hpp>
#include <piranha/utils/stack_trace.hpp>

namespace piranha_test
{

// Stack trace generation on some setups can be quite
// slow. This helper can be used to disable stack traces
// at runtime if needed.
inline void disable_slow_stack_traces()
{
#if defined(PIRANHA_WITH_STACK_TRACES) && defined(_WIN32)
    piranha::set_stack_trace_enabled(false);
#endif
}

} // namespace piranha_test

#endif
