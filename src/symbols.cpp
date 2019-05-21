// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>

#include <piranha/symbols.hpp>

namespace piranha::detail
{

// Get a string representation of a symbol_set.
::std::string to_string(const symbol_set &s)
{
    ::std::string retval = "{";
    for (auto it = s.begin(); it != s.end(); ++it) {
        retval += '\'';
        retval += *it;
        retval += '\'';
        if (it + 1 != s.end()) {
            retval += ", ";
        }
    }
    retval += '}';

    return retval;
}

} // namespace piranha::detail
