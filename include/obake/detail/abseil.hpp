// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_ABSEIL_HPP
#define OBAKE_DETAIL_ABSEIL_HPP

// NOTE: on MSVC, some abseil headers give
// warnings in debug mode, which make
// obake's debug builds fail. Suppress
// those warnings.
#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(push)
#pragma warning(disable : 4245)
#pragma warning(disable : 4127)
#pragma warning(disable : 4996)
#pragma warning(disable : 4324)

#endif

#include <absl/base/attributes.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <absl/numeric/int128.h>

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(pop)

#endif

#endif
