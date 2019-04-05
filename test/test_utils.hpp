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

#if defined(PIRANHA_WITH_STACK_TRACES)

#include <piranha/utils/stack_trace.hpp>

#endif

namespace piranha_test
{

inline void disable_stack_traces()
{
#if defined(PIRANHA_WITH_STACK_TRACES)
    piranha::set_stack_trace_enabled(false);
#endif
}

} // namespace piranha_test

#endif
