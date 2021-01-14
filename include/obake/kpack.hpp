// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_KPACK_HPP
#define OBAKE_KPACK_HPP

#include <obake/config.hpp>

#include <cassert>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#if defined(OBAKE_PACKABLE_INT64) && defined(_MSC_VER)

#include <intrin.h>

#endif

#include <fmt/format.h>

#include <obake/detail/visibility.hpp>
#include <obake/exceptions.hpp>
#include <obake/type_name.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

// Only allow a closed set of types to be kpackable. Currently,
// 32-bit integers are always supported and 64-bit are supported
// if a mulhi() primitive for std::uint64_t is available (which
// is generally the case on 64-bit archs).
namespace detail
{

template <typename>
struct is_kpackable_impl : ::std::false_type {
};

template <typename>
struct kpack_data {
};

// Helper to return the high half of the product
// of two unsigned integers. The default implementation
// causes a compile-time error.
template <typename T>
inline T mulhi(T, T)
{
    static_assert(always_false_v<T>);
}

// Specialisations for 32-bit integers.
template <>
struct is_kpackable_impl<::std::int32_t> : ::std::true_type {
};

template <>
struct is_kpackable_impl<::std::uint32_t> : ::std::true_type {
};

template <>
struct OBAKE_DLL_PUBLIC kpack_data<::std::int32_t> {
    // The list of deltas, one of each size starting from 1.
    static const ::std::int32_t deltas[10];
    // The components' limits in absolute value, one for each size.
    static const ::std::int32_t lims[10];
    // The coded value limits in absolute value, one for each size.
    static const ::std::int32_t klims[10];
    // The data necessary to divide by constants.
    // NOTE: it seems like the first shift value we produce
    // in divcnst is always 1. If we can prove that this is always
    // ensured, perhaps we can reduce the tuple size and hard-code
    // 1 in the unpacking code.
    static const ::std::tuple<std::uint32_t, unsigned, unsigned> divcnst[10][11];
};

template <>
struct OBAKE_DLL_PUBLIC kpack_data<::std::uint32_t> {
    static const ::std::uint32_t deltas[10];
    static const ::std::uint32_t lims[10];
    static const ::std::uint32_t klims[10];
    static const ::std::tuple<std::uint32_t, unsigned, unsigned> divcnst[10][11];
};

template <>
inline ::std::uint32_t mulhi(::std::uint32_t a, ::std::uint32_t b)
{
    return static_cast<::std::uint32_t>((::std::uint64_t(a) * b) >> 32);
}

#if defined(OBAKE_PACKABLE_INT64)

template <>
struct is_kpackable_impl<::std::int64_t> : ::std::true_type {
};

template <>
struct is_kpackable_impl<::std::uint64_t> : ::std::true_type {
};

template <>
struct OBAKE_DLL_PUBLIC kpack_data<::std::int64_t> {
    static const ::std::int64_t deltas[21];
    static const ::std::int64_t lims[21];
    static const ::std::int64_t klims[21];
    static const ::std::tuple<std::uint64_t, unsigned, unsigned> divcnst[21][22];
};

template <>
struct OBAKE_DLL_PUBLIC kpack_data<::std::uint64_t> {
    static const ::std::uint64_t deltas[21];
    static const ::std::uint64_t lims[21];
    static const ::std::uint64_t klims[21];
    static const ::std::tuple<std::uint64_t, unsigned, unsigned> divcnst[21][22];
};

template <>
inline ::std::uint64_t mulhi(::std::uint64_t a, ::std::uint64_t b)
{
    return
#if defined(OBAKE_HAVE_GCC_INT128)
        static_cast<::std::uint64_t>((__uint128_t(a) * b) >> 64);
#else
        __umulh(a, b);
#endif
}

#endif

} // namespace detail

template <typename T>
using is_kpackable = detail::is_kpackable_impl<T>;

template <typename T>
inline constexpr bool is_kpackable_v = is_kpackable<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL kpackable = is_kpackable_v<T>;

#endif

namespace detail
{

// Various helpers to fetch kpack data.

// Return the max packable size for a given type. This is
// the size of the deltas, lims and klims arrays.
// NOTE: unsigned conversion is ok as the array sizes
// are always a fraction of a bit size.
template <typename T>
constexpr unsigned kpack_max_size()
{
    auto ret = ::std::size(kpack_data<T>::deltas);

    assert(ret == ::std::size(kpack_data<T>::lims));
    assert(ret == ::std::size(kpack_data<T>::klims));

    return static_cast<unsigned>(ret);
}

// Return the delta for a given size.
template <typename T>
inline T kpack_get_delta(unsigned size)
{
    assert(size > 0u && size <= detail::kpack_max_size<T>());

    return kpack_data<T>::deltas[size - 1u];
}

// Return the components' limits for a given size.
template <typename T>
inline ::std::pair<T, T> kpack_get_lims(unsigned size)
{
    assert(size > 0u && size <= detail::kpack_max_size<T>());

    const auto lim = kpack_data<T>::lims[size - 1u];

    if constexpr (is_signed_v<T>) {
        return ::std::pair{-lim, lim};
    } else {
        return ::std::pair{T(0), lim};
    }
}

// Return the coded values' limits for a given size.
template <typename T>
inline ::std::pair<T, T> kpack_get_klims(unsigned size)
{
    assert(size > 0u && size <= detail::kpack_max_size<T>());

    const auto klim = kpack_data<T>::klims[size - 1u];

    if constexpr (is_signed_v<T>) {
        return ::std::pair{-klim, klim};
    } else {
        return ::std::pair{T(0), klim};
    }
}

} // namespace detail

// Kronecker packer.
#if defined(OBAKE_HAVE_CONCEPTS)
template <kpackable T>
#else
template <typename T, typename = ::std::enable_if_t<is_kpackable_v<T>>>
#endif
class kpacker
{
    T m_value = 0;
    T m_cur_prod = 1;
    unsigned m_index = 0;
    unsigned m_size;

public:
    // Constructor from size.
    explicit kpacker(unsigned size) : m_size(size)
    {
        if (obake_unlikely(size > detail::kpack_max_size<T>())) {
            using namespace ::fmt::literals;

            obake_throw(::std::overflow_error,
                        "Invalid size specified in the constructor of a Kronecker packer for "
                        "the type '{}': the maximum possible size is {}, but a size of {} "
                        "was specified instead"_format(::obake::type_name<T>(), detail::kpack_max_size<T>(), size));
        }
    }

    // Insert the next value into the packer.
    kpacker &operator<<(const T &n)
    {
        using namespace ::fmt::literals;

        if (obake_unlikely(m_index == m_size)) {
            obake_throw(::std::out_of_range,
                        "Cannot push any more values to this Kronecker packer for the type '{}': the number of "
                        "values already pushed to the packer is equal to the packer's size ({})"_format(
                            ::obake::type_name<T>(), m_size));
        }

        // Get the components' limits.
        const auto [lim_min, lim_max] = detail::kpack_get_lims<T>(m_size);

        // Check that n is within the limits.
        if (obake_unlikely(n < lim_min || n > lim_max)) {
            obake_throw(::std::overflow_error, "Cannot push the value {} to this Kronecker packer for the type "
                                               "'{}': the value is outside the allowed range [{}, {}]"_format(
                                                   n, ::obake::type_name<T>(), lim_min, lim_max));
        }

        // Do the encoding.
        m_value += n * m_cur_prod;
        // Update the value of the current component
        // of the coding vector.
        m_cur_prod *= detail::kpack_get_delta<T>(m_size);
        ++m_index;

        return *this;
    }

    // Fetch the encoded value.
    const T &get() const
    {
        // NOTE: if we pushed fewer values than m_size,
        // this will be equivalent to having pushed
        // zeroes for the missing values.
        return m_value;
    }
};

// Kronecker unpacker.
#if defined(OBAKE_HAVE_CONCEPTS)
template <kpackable T>
#else
template <typename T, typename = ::std::enable_if_t<is_kpackable_v<T>>>
#endif
class kunpacker
{
    T m_value;
    T m_cur_prod = 1;
    unsigned m_index = 0;
    unsigned m_size;

public:
    explicit kunpacker(const T &n, unsigned size) : m_value(n), m_size(size)
    {
        using namespace ::fmt::literals;

        if (size == 0u) {
            if (obake_unlikely(n != T(0))) {
                obake_throw(::std::invalid_argument,
                            "Only a value of zero can be used in a Kronecker unpacker "
                            "with a size of zero, but a value of {} was provided instead"_format(n));
            }
        } else {
            if (obake_unlikely(size > detail::kpack_max_size<T>())) {
                obake_throw(::std::overflow_error,
                            "Invalid size specified in the constructor of a Kronecker unpacker for the type '{}': the "
                            "maximum possible size is {}, but a size of {} was specified instead"_format(
                                ::obake::type_name<T>(), detail::kpack_max_size<T>(), size));
            }

            // Get the coded value's limits.
            const auto [klim_min, klim_max] = detail::kpack_get_klims<T>(m_size);

            // Check that n is within the limits.
            if (obake_unlikely(n < klim_min || n > klim_max)) {
                obake_throw(::std::overflow_error, "The value {} passed to a Kronecker unpacker for the type "
                                                   "'{}' is outside the allowed range [{}, {}]"_format(
                                                       n, ::obake::type_name<T>(), klim_min, klim_max));
            }
        }
    }

    kunpacker &operator>>(T &out)
    {
        if (obake_unlikely(m_index == m_size)) {
            using namespace ::fmt::literals;

            obake_throw(::std::out_of_range,
                        "Cannot unpack any more values from this Kronecker unpacker: the number of "
                        "values already unpacked is equal to the unpacker's size ({})"_format(m_size));
        }

        // Prepare m_cur_prod. This is the divisor in the remainder operation.
        const auto delta = detail::kpack_get_delta<T>(m_size);
        m_cur_prod *= delta;

        // Fetch the data necessary for division and remainder.
        const auto [mp_d, sh1_d, sh2_d] = detail::kpack_data<T>::divcnst[m_size - 1u][m_index];
        const auto [mp_r, sh1_r, sh2_r] = detail::kpack_data<T>::divcnst[m_size - 1u][m_index + 1u];
        // NOTE: if mprime ends up being zero, it means we read into the wrong indices into divcnst.
        assert(mp_d != 0u);
        assert(mp_r != 0u);

        // Compute the shifted counterpart of m_value.
        const auto n = m_value - detail::kpack_get_klims<T>(m_size).first;

        // Helper to perform the division by a constant. This is the algorithm
        // in Figure 4.1 in:
        // https://gmplib.org/~tege/divcnst-pldi94.pdf
        // NOTE: the division is performed using the unsigned counterpart of T.
        using unsigned_t = make_unsigned_t<T>;
        auto divcnst = [](unsigned_t n, unsigned_t mp, unsigned sh1, unsigned sh2) {
            const auto t1 = detail::mulhi(mp, n);
            const auto tmp = (n - t1) >> sh1;
            return (t1 + tmp) >> sh2;
        };

        // Do the remainder part.
        const auto q_r = divcnst(static_cast<unsigned_t>(n), mp_r, sh1_r, sh2_r);
        assert(q_r == static_cast<unsigned_t>(n) / static_cast<unsigned_t>(m_cur_prod));
        const auto rem = static_cast<unsigned_t>(n) - q_r * static_cast<unsigned_t>(m_cur_prod);
        assert(rem == static_cast<unsigned_t>(n) % static_cast<unsigned_t>(m_cur_prod));

        // Do the division part.
        const auto q_d = static_cast<T>(divcnst(rem, mp_d, sh1_d, sh2_d));
        assert(q_d == static_cast<T>(rem) / (m_cur_prod / delta));

        // Write out the result.
        out = q_d + detail::kpack_get_lims<T>(m_size).first;

        ++m_index;

        return *this;
    }
};

} // namespace obake

#endif
