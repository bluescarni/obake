// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_IGNORE_HPP
#define PIRANHA_DETAIL_IGNORE_HPP

namespace piranha::detail
{

// Small helper to quench compiler warnings for
// maybe unused variables.
template <typename... Args>
constexpr void ignore(const Args &...)
{
}

} // namespace piranha::detail

#endif
