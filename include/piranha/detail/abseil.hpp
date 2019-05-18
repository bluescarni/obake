// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_ABSEIL_HPP
#define PIRANHA_DETAIL_ABSEIL_HPP

// NOTE: on MSVC, some abseil headers give
// warnings in debug mode, which make
// piranha's debug builds fail. Suppress
// those warnings.
#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(push)
#pragma warning(disable : 4245)
#pragma warning(disable : 4127)
#pragma warning(disable : 4996)

#endif

#include <absl/container/flat_hash_map.h>

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(pop)

#endif

#endif
