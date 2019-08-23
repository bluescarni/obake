// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_K_PACKING_HPP
#define PIRANHA_K_PACKING_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/detail/carray.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/detail/xoroshiro128_plus.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/utils/type_name.hpp>

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(push)
#pragma warning(disable : 4307)

#endif

namespace piranha
{

// Only allow certain integral types to be packable (this is due to the complications arising
// from integral promotion rules for short ints and char types).
template <typename T>
using is_k_packable = ::std::disjunction<::std::is_same<T, int>, ::std::is_same<T, unsigned>, ::std::is_same<T, long>,
                                         ::std::is_same<T, unsigned long>, ::std::is_same<T, long long>,
                                         ::std::is_same<T, unsigned long long>
#if defined(PIRANHA_HAVE_GCC_INT128)
                                         ,
                                         ::std::is_same<T, __int128_t>, ::std::is_same<T, __uint128_t>
#endif
                                         >;

template <typename T>
inline constexpr bool is_k_packable_v = is_k_packable<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL KPackable = is_k_packable_v<T>;

#endif

namespace detail
{

// Randomly generate the deltas for the k encoding.
// From the deltas, we will then deduce the components'
// limits, the coding vectors, etc.
//
// The strategy we follow is to dedicate a fixed number
// of bits N for each delta. We start from N = 3 up to
// bit_width - 1 (and not bit_width, because in that case
// we want to allow for only 1 component using the full range
// of the type). For each value of N, we deduce the size M of
// the coding vector (i.e., the number of N-bit components that can be
// packed in a singe instance of T) by enforcing that the product
// of all deltas must be still representable by T.
//
// E.g., if N == 3 and T is a 64-bit unsigned integer, then we have
// M == 64 / 3 == 21. In other words, by reserving 3 bits to each delta we
// ensure that the product of 21 deltas is representable by the type T.
// If T is signed, then we still have M == (64 - 1) / 3 == 21 (we removed
// the sign bit), thus with 21 components we are still sure that the product
// of all deltas is representable by the signed type (remember that all deltas are
// positive by construction).
//
// If we can represent with T the product of all deltas, it can be shown
// that we can also represent with T:
// - all the components of the coding vector (trivially),
// - the min/max encoded values, and their difference as well.
// This result relies on the assumptions that:
// - if T is signed, it uses two's complement (guaranteed from C++20, but already
//   always true in practice),
// - if T is unsigned, the min component value is always chosen zero,
// - if T is signed, the min component value is always chosen negative and the
//   max component value is always chosen positive.
template <typename T>
constexpr auto k_packing_compute_deltas()
{
    static_assert(is_integral_v<T>, "T must be an integral type.");

    // NOTE: the bit width for signed integral types does *not* include
    // the sign bit already.
    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= ::std::get<1>(limits_minmax<::std::size_t>), "Overflow error.");

    // Number of rows of the table that will be returned. Each row
    // corresponds to a different bit width for the deltas, starting
    // from 3 up to bit_width - 1.
    constexpr auto nrows = static_cast<::std::size_t>(bit_width - 3u);

    // Number of columns of the table. This is given by the highest
    // possible number of components, that is, the number of components
    // when the deltas are 3-bit wide.
    constexpr auto ncols = static_cast<::std::size_t>(bit_width / 3u);

    // Return value, structured as a table.
    // NOTE: only the first row of the returned table will be filled
    // with nonzero values. The other rows will be right-padded with zeroes
    // as the number of components decreases while the number of bits
    // for the deltas increases.
    carray<carray<T, ncols>, nrows> retval{};

    // Compile-time pseudo-random generator.
    // https://xkcd.com/221/
    xoroshiro128_plus rng{12724899751400538854ull, 9282269007213506749ull};

    for (::std::size_t i = 0; i < nrows; ++i) {
        // Current number of bits for the deltas.
        const auto cur_nbits = i + 3u;
        // Current number of components.
        const auto cur_ncols = bit_width / cur_nbits;

        for (::std::size_t j = 0; j < cur_ncols; ++j) {
            // NOTE: do the number generation using the unsigned
            // counterpart.
            using uint_t = make_unsigned_t<T>;

            const auto rnd = rng.template random<uint_t>();

            // The generated random number will have the top two bits
            // set to 1. This ensures that the generated deltas don't vary
            // too much, while still retaining some randomicity in the lower bits.
            const auto hi = T(3) << (cur_nbits - 2u);
            const auto lo = static_cast<T>(rnd >> (static_cast<unsigned>(limits_digits<uint_t>) - cur_nbits + 2u));

            retval[i][j] = hi + lo;
        }
    }

    return retval;
}

// Compute the coding vectors for the type T.
// The coding vectors are the partial products of the deltas,
// starting from a value of 1. Thus, the length of each coding vector
// is 1 + the number of deltas.
template <typename T>
constexpr auto k_packing_compute_cvs()
{
    static_assert(is_integral_v<T>, "T must be an integral type.");

    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= ::std::get<1>(limits_minmax<::std::size_t>), "Overflow error.");

    constexpr auto nrows = static_cast<::std::size_t>(bit_width - 3u);
    constexpr auto ncols = static_cast<::std::size_t>(bit_width / 3u + 1u);

    carray<carray<T, ncols>, nrows> retval{};

    constexpr auto deltas = detail::k_packing_compute_deltas<T>();

    for (::std::size_t i = 0; i < nrows; ++i) {
        const auto cur_nbits = i + 3u;
        const auto cur_ncols = bit_width / cur_nbits + 1u;

        retval[i][0] = 1u;

        for (::std::size_t j = 1; j < cur_ncols; ++j) {
            retval[i][j] = retval[i][j - 1u] * deltas[i][j - 1u];
        }
    }

    return retval;
}

// Compute the component limits for the type T.
template <typename T>
constexpr auto k_packing_compute_limits()
{
    static_assert(is_integral_v<T>, "T must be an integral type.");

    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= ::std::get<1>(limits_minmax<::std::size_t>), "Overflow error.");

    constexpr auto nrows = static_cast<::std::size_t>(bit_width - 3u);
    constexpr auto ncols = static_cast<::std::size_t>(bit_width / 3u);

    constexpr auto deltas = detail::k_packing_compute_deltas<T>();

    if constexpr (is_signed_v<T>) {
        carray<carray<carray<T, 2>, ncols>, nrows> retval{};

        for (::std::size_t i = 0; i < nrows; ++i) {
            const auto cur_nbits = i + 3u;
            const auto cur_ncols = bit_width / cur_nbits;

            for (::std::size_t j = 0; j < cur_ncols; ++j) {
                const auto delta = deltas[i][j];

                // NOTE: for signed integers, we choose two's-complement-style
                // limits if possible (i.e., even delta), or symmetric limits
                // (odd delta).
                if (delta % T(2)) {
                    retval[i][j][0] = (T(1) - delta) / T(2);
                    retval[i][j][1] = -retval[i][j][0];
                } else {
                    retval[i][j][0] = -delta / T(2);
                    retval[i][j][1] = delta / T(2) - T(1);
                }
            }
        }

        return retval;
    } else {
        carray<carray<T, ncols>, nrows> retval{};

        for (::std::size_t i = 0; i < nrows; ++i) {
            const auto cur_nbits = i + 3u;
            const auto cur_ncols = bit_width / cur_nbits;

            for (::std::size_t j = 0; j < cur_ncols; ++j) {
                assert(deltas[i][j] >= 1u);
                retval[i][j] = deltas[i][j] - T(1);
            }
        }

        return retval;
    }
}

// Compute the min/max encoded values.
template <typename T>
constexpr auto k_packing_compute_encoded_limits()
{
    static_assert(is_integral_v<T>, "T must be an integral type.");

    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= ::std::get<1>(limits_minmax<::std::size_t>), "Overflow error.");

    constexpr auto nrows = static_cast<::std::size_t>(bit_width - 3u);

    constexpr auto cvs = detail::k_packing_compute_cvs<T>();
    constexpr auto limits = detail::k_packing_compute_limits<T>();

    if constexpr (is_signed_v<T>) {
        carray<carray<T, 2>, nrows> retval{};

        for (::std::size_t i = 0; i < nrows; ++i) {
            const auto cur_nbits = i + 3u;
            const auto cur_ncols = bit_width / cur_nbits;

            T lim_min(0), lim_max(0);

            for (::std::size_t j = 0; j < cur_ncols; ++j) {
                lim_min += cvs[i][j] * limits[i][j][0];
                lim_max += cvs[i][j] * limits[i][j][1];
            }

            assert(lim_max > lim_min);

            retval[i][0] = lim_min;
            retval[i][1] = lim_max;
        }

        return retval;
    } else {
        carray<T, nrows> retval{};

        for (::std::size_t i = 0; i < nrows; ++i) {
            const auto cur_nbits = i + 3u;
            const auto cur_ncols = bit_width / cur_nbits;

            T lim(0);

            for (::std::size_t j = 0; j < cur_ncols; ++j) {
                lim += cvs[i][j] * limits[i][j];
            }

            retval[i] = lim;
        }

        return retval;
    }
}

// Package the data necessary for encoding/decoding in a compile-time tuple,
// so that we can take advantage of constexpr's UB checking (e.g., for signed
// overflows, out of bounds read writes, etc.).
template <typename T>
inline constexpr auto k_packing_data
    = ::std::make_tuple(detail::k_packing_compute_deltas<T>(), detail::k_packing_compute_cvs<T>(),
                        detail::k_packing_compute_limits<T>(), detail::k_packing_compute_encoded_limits<T>());

} // namespace detail

// Kronecker packer.
template <typename T>
class k_packer
{
public:
    // Constructor from size.
    constexpr explicit k_packer(unsigned size) : m_value(0), m_index(0), m_size(size), m_data_idx(0)
    {
        if (size) {
            const auto nbits = static_cast<unsigned>(detail::limits_digits<T>) / size;
            if (piranha_unlikely(nbits < 3u)) {
                piranha_throw(::std::invalid_argument,
                              "Invalid size specified in the constructor of a Kronecker packer for the type '"
                                  + ::piranha::type_name<T>() + "': the maximum possible size is "
                                  + detail::to_string(detail::limits_digits<T> / 3) + ", but a size of "
                                  + detail::to_string(size) + " was specified instead");
            }

            // NOTE: m_data_idx corresponds to the row index of the
            // k_packing table data. m_index corresponds to the column
            // index.
            m_data_idx = nbits - 3u;
        }
    }
    // Insert the next value into the packer.
    constexpr k_packer &operator<<(const T &n)
    {
        if (piranha_unlikely(m_index == m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot push any more values to this Kronecker packer: the number of "
                          "values already pushed to the packer is equal to the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }

        // Special case for size 1: in that case, we don't
        // pack anything and just employ the full range of T.
        if (m_size == 1u) {
            m_value = n;
            ++m_index;

            return *this;
        }

        assert(m_data_idx < ::std::get<2>(detail::k_packing_data<T>).size());
        assert(m_index < ::std::get<2>(detail::k_packing_data<T>)[m_data_idx].size());

        // Get the limits of the components.
        const auto &lims = ::std::get<2>(detail::k_packing_data<T>)[m_data_idx][m_index];

        // Check that n is within the allowed limits for the current component.
        if constexpr (is_signed_v<T>) {
            if (piranha_unlikely(n < lims[0] || n > lims[1])) {
                piranha_throw(::std::overflow_error,
                              "Cannot push the value " + detail::to_string(n)
                                  + " to this Kronecker packer: the value is outside the allowed range ["
                                  + detail::to_string(lims[0]) + ", " + detail::to_string(lims[1]) + "]");
            }
        } else {
            if (piranha_unlikely(n > lims)) {
                piranha_throw(::std::overflow_error,
                              "Cannot push the value " + detail::to_string(n)
                                  + " to this Kronecker packer: the value is outside the allowed range [0, "
                                  + detail::to_string(lims) + "]");
            }
        }

        // Do the encoding.
        const auto &c_value = ::std::get<1>(detail::k_packing_data<T>)[m_data_idx][m_index];
        m_value += n * c_value;
        ++m_index;

        return *this;
    }

    // Fetch the encoded value.
    constexpr const T &get() const
    {
        if (piranha_unlikely(m_index < m_size)) {
            piranha_throw(::std::out_of_range,
                          "Cannot fetch the packed value from this Kronecker packer: the number of "
                          "values pushed to the packer ("
                              + detail::to_string(m_index) + ") is less than the size used for construction ("
                              + detail::to_string(m_size) + ")");
        }
        return m_value;
    }

private:
    T m_value;
    unsigned m_index;
    unsigned m_size;
    unsigned m_data_idx;
};

} // namespace piranha

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(pop)

#endif

#endif
