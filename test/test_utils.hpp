// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_TEST_UTILS_HPP
#define PIRANHA_TEST_UTILS_HPP

#include <boost/algorithm/string/predicate.hpp>

#include <piranha/config.hpp>
#include <piranha/stack_trace.hpp>

#include "catch.hpp"

namespace piranha_test
{

// Stack trace generation on some setups can be quite
// slow. This helper can be used to disable stack traces
// at runtime if needed.
inline void disable_slow_stack_traces()
{
#if !defined(PIRANHA_WITH_LIBBACKTRACE) && defined(_WIN32)
    // NOTE: stack traces on Windows are very slow (unless using
    // libbacktrace with MinGW).
    piranha::set_stack_trace_enabled(false);
#endif
}

} // namespace piranha_test

#define PIRANHA_REQUIRES_THROWS_CONTAINS(expr, exc, msg)                                                               \
    try {                                                                                                              \
        expr;                                                                                                          \
        /* Exception not thrown. */                                                                                    \
        REQUIRE(false);                                                                                                \
    } catch (const exc &e) {                                                                                           \
        REQUIRE(boost::contains(e.what(), msg));                                                                       \
    } catch (...) {                                                                                                    \
        /* Wrong exception type. */                                                                                    \
        REQUIRE(false);                                                                                                \
    }

#endif
