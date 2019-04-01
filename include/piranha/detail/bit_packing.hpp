// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_BIT_PACKING_HPP
#define PIRANHA_DETAIL_BIT_PACKING_HPP

#include <cstddef>
#include <stdexcept>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/type_traits.hpp>

namespace piranha::detail
{

template <typename T>
using is_bit_packable = ::std::disjunction<::std::is_same<T, int>, ::std::is_same<T, unsigned>, ::std::is_same<T, long>,
                                           ::std::is_same<T, unsigned long>, ::std::is_same<T, long long>,
                                           ::std::is_same<T, unsigned long long>
#if defined(PIRANHA_HAVE_GCC_INT128)
                                           ,
                                           ::std::is_same<T, __int128_t>, ::std::is_same<T, __uint128_t>
#endif
                                           >;

template <typename T>
inline constexpr bool is_bit_packable_v = is_bit_packable<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL BitPackable = is_bit_packable_v<T>;

#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T>
requires BitPackable<T>
#else
template <typename T, typename = ::std::enable_if_t<is_bit_packable_v<T>>>
#endif
    class bit_packer
{
public:
    using value_type = make_unsigned_t<T>;
    explicit bit_packer(unsigned size)
        : m_value(0), m_max(0), m_s_offset(0), m_index(0), m_size(size), m_pbits(0), m_cur_shift(0)
    {
        constexpr auto nbits = static_cast<unsigned>(limits_digits<value_type>);
        if (piranha_unlikely(size > nbits)) {
            piranha_throw(::std::overflow_error, "");
        }
        if (size) {
            m_pbits = nbits / size;
            m_max = static_cast<value_type>(-1) >> (nbits - m_pbits);
            if constexpr (is_signed_v<T>) {
                m_s_offset = value_type(1) << (m_pbits - 1u);
            }
        }
    }
    bit_packer &operator<<(const T &n)
    {
        if (piranha_unlikely(m_index == m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot push any more values to this bit packer: the number of "
                          "values already pushed to the packer is equal to the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }
        const auto shift_n = [&n, this]() {
            if constexpr (is_signed_v<T>) {
                return static_cast<value_type>(n) + m_s_offset;
            } else {
                return n;
            }
        }();
        if (piranha_unlikely(shift_n > m_max)) {
            piranha_throw(::std::overflow_error, "");
        }
        m_value += shift_n << m_cur_shift;
        ++m_index;
        m_cur_shift += m_pbits;
        return *this;
    }

private:
    value_type m_value, m_max, m_s_offset;
    unsigned m_index, m_size, m_pbits, m_cur_shift;
};

} // namespace piranha::detail

#endif
