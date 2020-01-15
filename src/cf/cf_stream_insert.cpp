// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <ostream>

#include <obake/cf/cf_stream_insert.hpp>
#include <obake/config.hpp>
#include <obake/detail/to_string.hpp>

namespace obake::detail
{

#if defined(OBAKE_HAVE_GCC_INT128)

void cf_stream_insert(::std::ostream &os, const __int128_t &n)
{
    os << detail::to_string(n);
}

void cf_stream_insert(::std::ostream &os, const __uint128_t &n)
{
    os << detail::to_string(n);
}

#endif

} // namespace obake::detail
