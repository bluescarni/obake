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
#include <stdexcept>
#include <type_traits>

#include <boost/container/small_vector.hpp>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/k_packing.hpp>
#include <obake/symbols.hpp>

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
    // The number of exponents to be packed into each T instance.
    static constexpr unsigned psize = static_cast<unsigned>(::obake::detail::limits_digits<T>) / NBits;

    // Put a cap on the maximum number of elements in m_container,
    // ensuring also that max_v_size * psize is still representable by unsigned.
    static constexpr auto max_v_size = ::std::min(256u, ::obake::detail::limits_max<unsigned> / psize);

public:
    // Alias for T.
    using value_type = T;

    // Default constructor.
    d_packed_monomial() = default;

    // Constructor from symbol set.
    explicit d_packed_monomial(const symbol_set &ss)
    {
        const auto v_size = ss.size() / psize;
        if (obake_unlikely(v_size > max_v_size)) {
            obake_throw(
                ::std::invalid_argument,
                "Cannot create a dynamic packed monomial from the input symbol set: the size of the symbol set ("
                    + ::obake::detail::to_string(ss.size())
                    + ") overflows the maximum allowed size for the container of packed exponents ("
                    + ::obake::detail::to_string(max_v_size) + ")");
        }
        m_container.resize(v_size);
    }

private:
    ::boost::container::small_vector<T, 1> m_container;
};

} // namespace polynomials

template <typename T, unsigned NBits>
using d_packed_monomial = polynomials::d_packed_monomial<T, NBits>;

} // namespace obake

#endif
