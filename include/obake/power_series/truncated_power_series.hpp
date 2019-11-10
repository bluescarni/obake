// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POWER_SERIES_TRUNCATED_POWER_SERIES_HPP
#define OBAKE_POWER_SERIES_TRUNCATED_POWER_SERIES_HPP

#include <tuple>
#include <utility>

#include <boost/variant/variant.hpp>

#include <obake/math/degree.hpp>
#include <obake/math/p_degree.hpp>
#include <obake/polynomials/polynomial.hpp>
#include <obake/symbols.hpp>

namespace obake
{

namespace power_series
{

namespace detail
{

// Struct to represent the absence
// of truncation in a power series.
struct no_truncation {
};

} // namespace detail

// TODO type requirements for K and C:
// - must be suitable for use in polynomial,
// - poly_t must have total degree, and the degree
//   type must be suitable for use in variant,
// - same for the partial degree.
template <typename K, typename C>
class truncated_power_series
{
public:
    // Useful typedefs.
    using poly_t = polynomial<K, C>;
    using degree_t = decltype(::obake::degree(::std::declval<const poly_t &>()));
    using p_degree_t
        = decltype(::obake::p_degree(::std::declval<const poly_t &>(), ::std::declval<const symbol_set &>()));

private:
    // The truncation setting type.
    using trunc_t = ::boost::variant<detail::no_truncation, degree_t, ::std::tuple<p_degree_t, symbol_set>>;

public:
    truncated_power_series() = default;

    // The polynomial getters.
    poly_t &_poly()
    {
        return m_poly;
    }
    const poly_t &_poly() const
    {
        return m_poly;
    }

private:
    poly_t m_poly;
    trunc_t m_trunc;
};

template <typename K, typename C>
inline auto degree(const truncated_power_series<K, C> &tps)
{
    return ::obake::degree(tps._poly());
}

template <typename K, typename C>
inline auto p_degree(const truncated_power_series<K, C> &tps, const symbol_set &s)
{
    return ::obake::p_degree(tps._poly(), s);
}

} // namespace power_series

template <typename K, typename C>
using truncated_power_series = power_series::truncated_power_series<K, C>;

} // namespace obake

#endif
