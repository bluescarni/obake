// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_HC_HPP
#define OBAKE_DETAIL_HC_HPP

#include <obake/detail/visibility.hpp>

namespace obake::detail
{

// Return the hardware concurrency, i.e.,
// the number of logical cores on the system.
OBAKE_DLL_PUBLIC unsigned hc();

} // namespace obake::detail

#endif
