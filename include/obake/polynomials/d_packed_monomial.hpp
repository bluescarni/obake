// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)::polynomials
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POLYNOMIALS_D_PACKED_MONOMIAL_HPP
#define OBAKE_POLYNOMIALS_D_PACKED_MONOMIAL_HPP

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>

#include <boost/container/small_vector.hpp>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/k_packing.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace polynomials
{

// Dynamic packed monomial.
#if defined(OBAKE_HAVE_CONCEPTS)
template <KPackable T, unsigned NBits>
    requires(NBits >= 3u)
    && (NBits <= static_cast<unsigned>(::obake::detail::limits_digits<T>))
#else
template <typename T, unsigned NBits,
          typename = ::std::enable_if_t<is_k_packable_v<T> && (NBits >= 3u)
                                        && (NBits <= static_cast<unsigned>(::obake::detail::limits_digits<T>))>>
#endif
        class d_packed_monomial
{
public:
    // The number of exponents to be packed into each T instance.
    static constexpr unsigned psize = static_cast<unsigned>(::obake::detail::limits_digits<T>) / NBits;

private:
    // Small helper to determine the container size
    // we need to store n exponents. U must be an
    // unsigned integral type.
    template <typename U>
    static constexpr auto nexpos_to_vsize(const U &n) noexcept
    {
        static_assert(is_integral_v<U> && !is_signed_v<U>);
        return n / psize + static_cast<U>(n % psize != 0u);
    }

public:
    // Alias for T.
    using value_type = T;

    // The container type.
    using container_t = ::boost::container::small_vector<T, 1>;

    // Default constructor.
    d_packed_monomial() = default;

    // Constructor from symbol set.
    explicit d_packed_monomial(const symbol_set &ss)
        : m_container(::obake::safe_cast<typename container_t::size_type>(nexpos_to_vsize(ss.size())))
    {
    }

    // Constructor from input iterator and size.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <typename It>
    requires InputIterator<It> &&SafelyCastable<typename ::std::iterator_traits<It>::reference, T>
#else
    template <
        typename It,
        ::std::enable_if_t<::std::conjunction_v<is_input_iterator<It>,
                                                is_safely_castable<typename ::std::iterator_traits<It>::reference, T>>,
                           int> = 0>
#endif
        explicit d_packed_monomial(It it, ::std::size_t n)
    {
        const auto vsize = nexpos_to_vsize(n);
        m_container.resize(::obake::safe_cast<typename container_t::size_type>(vsize));
        auto it_c = m_container.begin();

        ::std::size_t counter = 0;
        for (auto i = 0u; i < vsize; ++i, ++it_c) {
            k_packer<T> kp(psize);

            auto j = 0u;
            for (; j < psize && counter < n; ++j, ++counter, ++it) {
                kp << ::obake::safe_cast<T>(*it);
            }

            for (; j < psize; ++j) {
                kp << T(0);
            }

            *it_c = kp.get();
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

private:
    container_t m_container;
};

// Comparisons.
template <typename T, unsigned NBits>
inline bool operator==(const d_packed_monomial<T, NBits> &d1, const d_packed_monomial<T, NBits> &d2)
{
    return d1._container() == d2._container();
}

template <typename T, unsigned NBits>
inline bool operator!=(const d_packed_monomial<T, NBits> &d1, const d_packed_monomial<T, NBits> &d2)
{
    return !(d1 == d2);
}

} // namespace polynomials

// Lift to the obake namespace.
template <typename T, unsigned NBits>
using d_packed_monomial = polynomials::d_packed_monomial<T, NBits>;

} // namespace obake

#endif
