// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_HC_HPP
#define PIRANHA_DETAIL_HC_HPP

#include <piranha/detail/visibility.hpp>

namespace piranha::detail
{

// Return the hardware concurrency, i.e.,
// the number of logical cores on the system.
PIRANHA_DLL_PUBLIC unsigned hc();

} // namespace piranha::detail

#endif
