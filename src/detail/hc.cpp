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

unsigned hc()
{
#if defined(PIRANHA_WITH_TBB)
    const static thread_local unsigned candidate = []() {
        const auto ret = ::std::thread::hardware_concurrency();
        return ret ? ret : 1u;
    }();

    return candidate;
#else
    return 1;
#endif
}

} // namespace piranha::detail
