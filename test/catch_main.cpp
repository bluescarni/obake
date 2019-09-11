// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Minimal main file to reduce catch compile times:
// https://github.com/catchorg/Catch2/blob/master/docs/slow-compiles.md

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_FAST_COMPILE

#if defined(_MSC_VER) && !defined(__clang__)

#define CATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#endif

#include "catch.hpp"
