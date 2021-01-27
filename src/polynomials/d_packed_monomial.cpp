// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdint>
#include <ostream>

#include <obake/config.hpp>
#include <obake/detail/visibility.hpp>
#include <obake/polynomials/d_packed_monomial.hpp>
#include <obake/symbols.hpp>

namespace obake
{

namespace polynomials
{

template OBAKE_DLL_PUBLIC void key_stream_insert(::std::ostream &, const d_packed_monomial<::std::int32_t, 8> &,
                                                 const symbol_set &);
template OBAKE_DLL_PUBLIC void key_stream_insert(::std::ostream &, const d_packed_monomial<::std::uint32_t, 8> &,
                                                 const symbol_set &);

#if defined(OBAKE_PACKABLE_INT64)

template OBAKE_DLL_PUBLIC void key_stream_insert(::std::ostream &, const d_packed_monomial<::std::int64_t, 8> &,
                                                 const symbol_set &);
template OBAKE_DLL_PUBLIC void key_stream_insert(::std::ostream &, const d_packed_monomial<::std::uint64_t, 8> &,
                                                 const symbol_set &);

#endif

} // namespace polynomials

} // namespace obake
