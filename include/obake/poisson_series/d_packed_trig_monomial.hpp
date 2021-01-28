// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)::polynomials
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POISSON_SERIES_D_PACKED_TRIG_MONOMIAL_HPP
#define OBAKE_POISSON_SERIES_D_PACKED_TRIG_MONOMIAL_HPP

#include <cstddef>
#include <iterator>
#include <stdexcept>

#include <boost/container/small_vector.hpp>

#include <fmt/format.h>

#include <obake/config.hpp>
#include <obake/exceptions.hpp>
#include <obake/kpack.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace poisson_series
{

namespace detail
{

// Small helper to determine the container size
// we need to store n exponents in a dynamic packed
// trig monomial of type T. U must be an
// unsigned integral type.
template <typename T, typename U>
constexpr auto dptm_n_expos_to_vsize(const U &n) noexcept
{
    static_assert(is_integral_v<U> && !is_signed_v<U>);
    return n / T::psize + static_cast<U>(n % T::psize != 0u);
}

} // namespace detail

// Max psize for d_packed_trig_monomial.
template <kpackable T>
requires(is_signed_v<T> == true) inline constexpr unsigned dptm_max_psize = ::obake::detail::kpack_max_size<T>();

// Dynamic packed trigonometric monomial.
template <kpackable T, unsigned PSize>
    requires(is_signed_v<T> == true) && (PSize > 0u) && (PSize <= dptm_max_psize<T>)class d_packed_trig_monomial
{
public:
    // The container type.
    using container_t = ::boost::container::small_vector<T, 1>;

private:
    container_t m_container;
    // true -> cosine, false -> sine.
    bool m_type = true;

public:
    // Alias for PSize
    static constexpr unsigned psize = PSize;

    // Alias for T.
    using value_type = T;

    // Default constructor.
    d_packed_trig_monomial() = default;

    // Constructor from symbol set.
    explicit d_packed_trig_monomial(const symbol_set &ss, bool type = true)
        : m_container(::obake::safe_cast<typename container_t::size_type>(
            detail::dptm_n_expos_to_vsize<d_packed_trig_monomial>(ss.size()))),
          m_type(type)
    {
    }

    // Constructor from input iterator and size.
    template <typename It>
    requires InputIterator<It> &&
        SafelyCastable<typename ::std::iterator_traits<It>::reference, T> explicit d_packed_trig_monomial(
            It it, ::std::size_t n, bool type = true)
        : m_type(type)
    {
        // Prepare the container.
        const auto vsize = detail::dptm_n_expos_to_vsize<d_packed_trig_monomial>(n);
        m_container.resize(::obake::safe_cast<typename container_t::size_type>(vsize));

        ::std::size_t counter = 0;
        bool first_nz_found = false;
        for (auto &out : m_container) {
            kpacker<T> kp(psize);

            // Keep packing until we get to psize or we have
            // exhausted the input values.
            for (auto j = 0u; j < psize && counter < n; ++j, ++counter, ++it) {
                const auto tmp = ::obake::safe_cast<T>(*it);

                if (obake_unlikely(!first_nz_found && tmp < 0)) {
                    // This is the first nonzero exponent
                    // and it is negative, thus the canonical
                    // form of the monomial is violated.
                    using namespace ::fmt::literals;
                    obake_throw(::std::invalid_argument,
                                "Cannot construct a trigonometric monomial whose first nonzero "
                                "exponent ({}) is negative"_format(tmp));
                }

                // Update first_nz_found.
                first_nz_found = (first_nz_found || tmp != 0);

                kp << tmp;
            }

            out = kp.get();
        }
    }

    container_t &_container()
    {
        return m_container;
    }
    const container_t &_container() const
    {
        return m_container;
    }

    bool &_type()
    {
        return m_type;
    }
    const bool &type() const
    {
        return m_type;
    }
};

} // namespace poisson_series

// Lift to the obake namespace.
template <typename T, unsigned PSize>
using d_packed_trig_monomial = poisson_series::d_packed_trig_monomial<T, PSize>;

} // namespace obake

#endif
