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
#include <tuple>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

#include <piranha/detail/visibility.hpp>

namespace piranha
{

// Set of symbols.
using symbol_set = ::boost::container::flat_set<::std::string>;

// Unsigned integral type for indexing into a symbol_set.
using symbol_idx = symbol_set::size_type;

// Set of symbol indices.
using symbol_idx_set = ::boost::container::flat_set<symbol_idx>;

// Map of symbol indices. This sorted data structure maps
// symbol_idx instances to instances of T.
template <typename T>
using symbol_idx_map = ::boost::container::flat_map<symbol_idx, T>;

namespace detail
{

PIRANHA_DLL_PUBLIC ::std::string to_string(const symbol_set &);

PIRANHA_DLL_PUBLIC ::std::tuple<symbol_set, symbol_idx_map<symbol_set>, symbol_idx_map<symbol_set>>
merge_symbol_sets(const symbol_set &, const symbol_set &);

PIRANHA_DLL_PUBLIC symbol_idx_set ss_intersect_idx(const symbol_set &, const symbol_set &);

} // namespace detail

} // namespace piranha

#endif
