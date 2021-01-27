// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdint>
#include <ostream>
#include <utility>
#include <vector>

#include <obake/config.hpp>
#include <obake/detail/visibility.hpp>
#include <obake/polynomials/d_packed_monomial.hpp>
#include <obake/symbols.hpp>

namespace obake::polynomials
{

template OBAKE_DLL_PUBLIC void
key_stream_insert(::std::ostream &, const d_packed_monomial<dpm_default_s_t, dpm_default_psize> &, const symbol_set &);

template OBAKE_DLL_PUBLIC void
key_stream_insert(::std::ostream &, const d_packed_monomial<dpm_default_u_t, dpm_default_psize> &, const symbol_set &);

template OBAKE_DLL_PUBLIC void key_tex_stream_insert(::std::ostream &,
                                                     const d_packed_monomial<dpm_default_s_t, dpm_default_psize> &,
                                                     const symbol_set &);

template OBAKE_DLL_PUBLIC void key_tex_stream_insert(::std::ostream &,
                                                     const d_packed_monomial<dpm_default_u_t, dpm_default_psize> &,
                                                     const symbol_set &);

template OBAKE_DLL_PUBLIC d_packed_monomial<dpm_default_s_t, dpm_default_psize>
key_merge_symbols(const d_packed_monomial<dpm_default_s_t, dpm_default_psize> &, const symbol_idx_map<symbol_set> &,
                  const symbol_set &);

template OBAKE_DLL_PUBLIC d_packed_monomial<dpm_default_u_t, dpm_default_psize>
key_merge_symbols(const d_packed_monomial<dpm_default_u_t, dpm_default_psize> &, const symbol_idx_map<symbol_set> &,
                  const symbol_set &);

template OBAKE_DLL_PUBLIC dpm_default_s_t key_degree(const d_packed_monomial<dpm_default_s_t, dpm_default_psize> &,
                                                     const symbol_set &);

template OBAKE_DLL_PUBLIC dpm_default_u_t key_degree(const d_packed_monomial<dpm_default_u_t, dpm_default_psize> &,
                                                     const symbol_set &);

template OBAKE_DLL_PUBLIC dpm_default_s_t key_p_degree(const d_packed_monomial<dpm_default_s_t, dpm_default_psize> &,
                                                       const symbol_idx_set &, const symbol_set &);

template OBAKE_DLL_PUBLIC dpm_default_u_t key_p_degree(const d_packed_monomial<dpm_default_u_t, dpm_default_psize> &,
                                                       const symbol_idx_set &, const symbol_set &);

template OBAKE_DLL_PUBLIC void key_trim_identify(::std::vector<int> &,
                                                 const d_packed_monomial<dpm_default_s_t, dpm_default_psize> &,
                                                 const symbol_set &);

template OBAKE_DLL_PUBLIC void key_trim_identify(::std::vector<int> &,
                                                 const d_packed_monomial<dpm_default_u_t, dpm_default_psize> &,
                                                 const symbol_set &);

template OBAKE_DLL_PUBLIC d_packed_monomial<dpm_default_s_t, dpm_default_psize>
key_trim(const d_packed_monomial<dpm_default_s_t, dpm_default_psize> &, const symbol_idx_set &, const symbol_set &);

template OBAKE_DLL_PUBLIC d_packed_monomial<dpm_default_u_t, dpm_default_psize>
key_trim(const d_packed_monomial<dpm_default_u_t, dpm_default_psize> &, const symbol_idx_set &, const symbol_set &);

template OBAKE_DLL_PUBLIC ::std::pair<dpm_default_s_t, d_packed_monomial<dpm_default_s_t, dpm_default_psize>>
monomial_diff(const d_packed_monomial<dpm_default_s_t, dpm_default_psize> &, const symbol_idx &, const symbol_set &);

template OBAKE_DLL_PUBLIC ::std::pair<dpm_default_u_t, d_packed_monomial<dpm_default_u_t, dpm_default_psize>>
monomial_diff(const d_packed_monomial<dpm_default_u_t, dpm_default_psize> &, const symbol_idx &, const symbol_set &);

template OBAKE_DLL_PUBLIC ::std::pair<dpm_default_s_t, d_packed_monomial<dpm_default_s_t, dpm_default_psize>>
monomial_integrate(const d_packed_monomial<dpm_default_s_t, dpm_default_psize> &, const symbol_idx &,
                   const symbol_set &);

template OBAKE_DLL_PUBLIC ::std::pair<dpm_default_u_t, d_packed_monomial<dpm_default_u_t, dpm_default_psize>>
monomial_integrate(const d_packed_monomial<dpm_default_u_t, dpm_default_psize> &, const symbol_idx &,
                   const symbol_set &);

} // namespace obake::polynomials
