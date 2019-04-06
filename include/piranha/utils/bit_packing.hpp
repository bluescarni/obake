// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_UTILS_BIT_PACKING_HPP
#define PIRANHA_UTILS_BIT_PACKING_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/detail/ignore.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/detail/visibility.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/type_traits.hpp>

// NOTE:
// - in these classes, we are exploiting two's complement representation when dealing with signed
//   integers. This is not guaranteed by the standard before C++20, but in practice even before
//   C++20 essentially all C++ implementations support only two's complement;
// - we have a few integral divisions/modulo operations in these classes which could probably
//   be replaced with lookup tables, should the need arise in terms of performance.

namespace piranha
{

// Only allow certain integral types to be packable (this is due to the complications arising
// from integral promotion rules for short ints and char types).
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

#if 0

#if defined(PIRANHA_HAVE_CONCEPTS)
template <typename T>
requires BitPackable<T>
#else
template <typename T, typename = void,
          typename = ::std::enable_if_t<::std::conjunction_v<is_bit_packable<T>, ::std::negation<is_signed<T>>>>>
#endif
    class bit_packer_
{
};

#endif

template <typename T>
class bit_packer_
{
public:
    constexpr explicit bit_packer_(unsigned size)
        : m_value(0), m_min(0), m_max(0), m_index(0), m_size(size), m_pbits(0), m_cur_shift(0)
    {
        constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<T> + 1);
        if (piranha_unlikely(size >= nbits)) {
            // TODO
            piranha_throw(::std::overflow_error, "The number of values to be pushed to this bit packer ("
                                                     + detail::to_string(size) + ") is larger than the bit width ("
                                                     + detail::to_string(nbits) + ") of the value type of the packer");
        }

        if (size) {
            if (size == 1u) {
                m_pbits = nbits;
                ::std::tie(m_min, m_max) = detail::limits_minmax<T>;
            } else {
                m_pbits = nbits / size - static_cast<unsigned>(nbits % size == 0u);
                assert(m_pbits);
                m_min = -(T(1) << (m_pbits - 1u));
                m_max = (T(1) << (m_pbits - 1u)) - T(1);
            }
        }
    }
    constexpr bit_packer_ &operator<<(const T &n)
    {
        if (piranha_unlikely(m_index == m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot push any more values to this bit packer: the number of "
                          "values already pushed to the packer is equal to the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }

        if (piranha_unlikely(n < m_min || n > m_max)) {
            // TODO
            piranha_throw(::std::overflow_error, "");
        }

        const auto tmp = (T(1) << m_cur_shift);
        m_value += n * tmp;
        ++m_index;
        m_cur_shift += m_pbits;

        return *this;
    }
    constexpr const T &get() const
    {
        if (piranha_unlikely(m_index < m_size)) {
            piranha_throw(::std::out_of_range, "Cannot fetch the packed value from this bit packer: the number of "
                                               "values pushed to the packer ("
                                                   + detail::to_string(m_index)
                                                   + ") is less than the size used for construction ("
                                                   + detail::to_string(m_size) + ")");
        }
        return m_value;
    }

private:
    T m_value, m_min, m_max;
    unsigned m_index, m_size, m_pbits, m_cur_shift;
};

namespace detail
{

template <typename T>
constexpr auto compute_minmax_packed()
{
    constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<T> + 1);

    ::std::array<::std::array<T, 2>, static_cast<unsigned>(detail::limits_digits<T>)> retval{};

    retval[0][0] = ::std::get<0>(detail::limits_minmax<T>);
    retval[0][1] = ::std::get<1>(detail::limits_minmax<T>);

    for (auto i = 1u; i < retval.size(); ++i) {
        const auto size = i + 1u;
        bit_packer_<T> bp_min(size), bp_max(size);
        const auto pbits = nbits / size - static_cast<unsigned>(nbits % size == 0u);
        const auto min = -(T(1) << (pbits - 1u)), max = (T(1) << (pbits - 1u)) - T(1);
        for (auto j = 0u; j < size; ++j) {
            bp_min << min;
            bp_max << max;
        }
        retval[i][0] = bp_min.get();
        retval[i][1] = bp_max.get();
    }

    return retval;
}

template <typename T>
struct foo_cont {
    PIRANHA_PUBLIC static const decltype(compute_minmax_packed<T>()) value;
};

template <>
const decltype(compute_minmax_packed<int>()) foo_cont<int>::value;

} // namespace detail

#if defined(PIRANHA_HAVE_CONCEPTS)
template <BitPackable T>
#else
template <typename T, typename = ::std::enable_if_t<is_bit_packable_v<T>>>
#endif
class bit_packer
{
public:
    // Use the unsigned counterpart of T for storage.
    using value_type = make_unsigned_t<T>;
    // Constructor from a number of values to be packed.
    explicit bit_packer(unsigned size)
        : m_value(0), m_max(0), m_s_offset(0), m_index(0), m_size(size), m_pbits(0), m_cur_shift(0)
    {
        constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<value_type>);
        if (piranha_unlikely(size > nbits)) {
            piranha_throw(::std::overflow_error, "The number of values to be pushed to this bit packer ("
                                                     + detail::to_string(size) + ") is larger than the bit width ("
                                                     + detail::to_string(nbits) + ") of the value type of the packer");
        }

        if (size) {
            // NOTE: compute these values only if we are actually going
            // to pack values (if size is zero, no packing ever happens).
            //
            // m_pbits is the number of bits that can be used by each
            // packed value.
            m_pbits = nbits / size;
            // m_max is the maximum packed value (in unsigned format).
            // It is a sequence of m_pbits 1 bits.
            m_max = static_cast<value_type>(-1) >> (nbits - m_pbits);
            if constexpr (is_signed_v<T>) {
                // NOTE: the signed offset is needed only if T is a signed type.
                //
                // This is the offset to be applied to the packed values
                // when T is signed. If we have m_pbits available for each
                // packed value, then the allowed range for signed packed values
                // is [-2**(m_pbits-1), 2**(m_pbits-1) - 1], same as two's
                // complement. We add 2**(m_pbits-1) to the signed packed values,
                // so that we obtain an unsigned range of [0, 2**m_pbits - 1].
                m_s_offset = value_type(1) << (m_pbits - 1u);
            }
        }
    }
    // Add the next value to be encoded.
    bit_packer &operator<<(const T &n)
    {
        if (piranha_unlikely(m_index == m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot push any more values to this bit packer: the number of "
                          "values already pushed to the packer is equal to the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }

        // Compute the shifted n: this is just n if T is unsigned,
        // otherwise we cast n to the unsigned counterpart and add the offset.
        // This will always lead to a shift_n value in the [0, 2**m_pbits - 1]
        // range.
        const auto shift_n = [&n, this]() {
            if constexpr (is_signed_v<T>) {
                return static_cast<value_type>(n) + m_s_offset;
            } else {
                detail::ignore(this);
                return n;
            }
        }();

        // Check the range of the input value. We can do the check
        // directly on the unsigned counterpart of n against the
        // unsigned max value.
        if (piranha_unlikely(shift_n > m_max)) {
            if constexpr (is_signed_v<T>) {
                // Convert the unsigned range to its signed counterpart.
                const auto range_max = static_cast<T>((value_type(1) << (m_pbits - 1u)) - 1u),
                           range_min = -range_max - T(1);
                piranha_throw(::std::overflow_error, "The signed value being pushed to this bit packer ("
                                                         + detail::to_string(n) + ") is outside the allowed range ["
                                                         + detail::to_string(range_min) + ", "
                                                         + detail::to_string(range_max) + "]");
            } else {
                piranha_throw(::std::overflow_error,
                              "The unsigned value being pushed to this bit packer (" + detail::to_string(shift_n)
                                  + ") is larger than the maximum allowed value (" + detail::to_string(m_max) + ")");
            }
        }

        // Do the actual packing (the new value will be
        // appended in the MSB direction).
        m_value += shift_n << m_cur_shift;
        ++m_index;
        m_cur_shift += m_pbits;

        return *this;
    }
    // Get the encoded value.
    value_type get() const
    {
        if (piranha_unlikely(m_index < m_size)) {
            piranha_throw(::std::out_of_range, "Cannot fetch the packed value from this bit packer: the number of "
                                               "values pushed to the packer ("
                                                   + detail::to_string(m_index)
                                                   + ") is less than the size used for construction ("
                                                   + detail::to_string(m_size) + ")");
        }
        return m_value;
    }

private:
    value_type m_value, m_max, m_s_offset;
    unsigned m_index, m_size, m_pbits, m_cur_shift;
};

#if 0

template <BitPackable T>
class bit_unpacker_
{
};

template <BitPackable T>
requires Signed<T> class bit_unpacker_<T>
{
public:
    constexpr explicit bit_unpacker_(const T &n, unsigned size) {}
};

#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
template <BitPackable T>
#else
template <typename T, typename = ::std::enable_if_t<is_bit_packable_v<T>>>
#endif
class bit_unpacker
{
public:
    using value_type = make_unsigned_t<T>;
    // Constructor from a packed value and a number of values to be unpacked.
    explicit bit_unpacker(const value_type &n, unsigned size)
        : m_value(n), m_mask(0), m_s_offset(0), m_index(0), m_size(size), m_pbits(0)
    {
        constexpr auto nbits = static_cast<unsigned>(detail::limits_digits<value_type>);
        if (piranha_unlikely(size > nbits)) {
            piranha_throw(::std::overflow_error, "The number of values to be extracted from this bit unpacker ("
                                                     + detail::to_string(size) + ") is larger than the bit width ("
                                                     + detail::to_string(nbits)
                                                     + ") of the value type of the unpacker");
        }
        if (size) {
            m_pbits = nbits / size;
            // The maximum decodable value is a sequence of m_pbits * size
            // one bits (starting from LSB).
            const auto max_decodable = value_type(-1) >> (nbits % size);
            if (piranha_unlikely(n > max_decodable)) {
                piranha_throw(::std::overflow_error, "The value to be unpacked (" + detail::to_string(n)
                                                         + ") is larger than the maximum allowed value ("
                                                         + detail::to_string(max_decodable) + ") for a range of size "
                                                         + detail::to_string(size));
            }
            // The mask for extracting the low m_pbits from a value.
            m_mask = value_type(-1) >> (nbits - m_pbits);
            if constexpr (is_signed_v<T>) {
                m_s_offset = value_type(1) << (m_pbits - 1u);
            }

            // NOTE: if size == 1 we set m_pbits back to zero.
            // The reason is that for size == 1 we would end up
            // downshifting m_value by nbits in operator>>() below,
            // which is undefined behaviour. Note that m_pbits
            // is only used in the downshifting from this point
            // onwards.
            if (size == 1u) {
                m_pbits = 0;
            }
        } else {
            if (piranha_unlikely(n)) {
                piranha_throw(::std::invalid_argument,
                              "Only a value of zero can be unpacked into an empty output range, but a value of "
                                  + detail::to_string(n) + " was provided instead");
            }
        }
    }
    bit_unpacker &operator>>(T &out)
    {
        if (piranha_unlikely(m_index == m_size)) {
            piranha_throw(::std::out_of_range, "Cannot unpack any more values from this bit unpacker: the number of "
                                               "values already unpacked is equal to the size used for construction ("
                                                   + detail::to_string(m_size) + ")");
        }

        // Unpack the current value and write it out.
        out = [this]() {
            if constexpr (is_signed_v<T>) {
                // NOTE: for signed values, we subtract the offset. Two's complement
                // conversion rules ensure that, if the original packed value was negative,
                // the wrap-around unsigned offset subtraction followed by a conversion
                // to signed recovers the original negative packed value.
                return static_cast<T>((m_value & m_mask) - m_s_offset);
            } else {
                return m_value & m_mask;
            }
        }();

        // Increase the index, shift down the current value.
        ++m_index;
        m_value >>= m_pbits;

        return *this;
    }

private:
    value_type m_value, m_mask, m_s_offset;
    unsigned m_index, m_size, m_pbits;
};

} // namespace piranha

#endif
