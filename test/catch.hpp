// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

// NOTE: helper to include the catch_impl.hpp
// header with pre-defined options (i.e., this avoids
// having to re-define the same catch options in
// every test file).
#define CATCH_CONFIG_FAST_COMPILE
#define CATCH_CONFIG_NO_CPP17_BYTE

#include "catch_impl.hpp"
