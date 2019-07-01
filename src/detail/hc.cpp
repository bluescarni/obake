// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/config.hpp>

#if defined(PIRANHA_WITH_TBB)

#include <thread>

#endif

#include <piranha/detail/hc.hpp>

namespace piranha::detail
{

// Return the hardware concurrency, i.e.,
// the number of logical cores on the system.
unsigned hc()
{
#if defined(PIRANHA_WITH_TBB)
    // NOTE: cache locally for each thread, so that we
    // avoid invoking a potentially costly function.
    const static thread_local unsigned retval = []() {
        const auto candidate = ::std::thread::hardware_concurrency();
        // If hardware_concurrency() fails, just return 1.
        return candidate ? candidate : 1u;
    }();

    return retval;
#else
    // Return always 1 if piranha is not configured
    // with support for TBB (in that case, we have no
    // use for detecting the hardware concurrency).
    return 1u;
#endif
}

} // namespace piranha::detail
