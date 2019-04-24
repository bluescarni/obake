// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_SYMBOLS_HPP
#define PIRANHA_SYMBOLS_HPP

#include <string>

#include <boost/container/flat_set.hpp>

namespace piranha
{

using symbol_set = ::boost::container::flat_set<::std::string>;

using symbol_idx = symbol_set::size_type;

} // namespace piranha

#endif
