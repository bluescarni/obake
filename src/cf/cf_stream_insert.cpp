// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <ostream>

#include <piranha/cf/cf_stream_insert.hpp>
#include <piranha/config.hpp>
#include <piranha/detail/to_string.hpp>

namespace piranha::detail
{

#if defined(MPPP_HAVE_GCC_INT128)

void cf_stream_insert(::std::ostream &os, const __int128_t &n)
{
    os << detail::to_string(n);
}

void cf_stream_insert(::std::ostream &os, const __uint128_t &n)
{
    os << detail::to_string(n);
}

#endif

} // namespace piranha::detail
