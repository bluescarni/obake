// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_TO_STRING_HPP
#define PIRANHA_DETAIL_TO_STRING_HPP

#include <string>

#include <piranha/config.hpp>
#include <piranha/detail/visibility.hpp>

namespace piranha::detail
{

template <typename T>
inline ::std::string to_string(const T &x)
{
    return ::std::to_string(x);
}

#if defined(PIRANHA_HAVE_GCC_INT128)

template <>
PIRANHA_PUBLIC ::std::string to_string(const __uint128_t &);

template <>
PIRANHA_PUBLIC ::std::string to_string(const __int128_t &);

#endif

} // namespace piranha::detail

#endif
