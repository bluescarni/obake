// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>

#include <obake/symbols.hpp>

#if defined(_WIN32) || defined(__CYGWIN__)
__declspec(dllexport)
#elif defined(__clang__) || defined(__GNUC__)
__attribute__((visibility("default")))
#endif
    std::string *get_test_address()
{
    return &::obake::detail::ss_fw_holder_class<std::string>::get();
}
