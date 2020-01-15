// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_K_PACKING_HPP
#define OBAKE_K_PACKING_HPP

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <obake/config.hpp>
#include <obake/detail/carray.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/xoroshiro128_plus.hpp>
#include <obake/exceptions.hpp>
#include <obake/type_name.hpp>

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(push)
#pragma warning(disable : 4307)

#endif

namespace obake
{

namespace detail
{

// Randomly generate the deltas for the k encoding.
// From the deltas, we will then deduce the components'
// limits, the coding vectors, etc.
//
// The strategy we follow is to dedicate a fixed number of bits N
// for each delta (the delta bit width). We start from N = 3 up to
// bit_width - 1 (and not bit_width, because in that case
// we want to allow for only 1 component using the full range
// of the type). For each value of N, we deduce the size M of
// the coding vector (i.e., the max number of N-bit components that can be
// packed in a single instance of T) by enforcing that the product
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
//
// Note that the table returned by this function has a row for each delta
// bit width, and that a delta bit width may correspond to more
// than one vector size. E.g., in a 64-bit unsigned integer, with a delta
// bit width of 3 bits we can have a vector size of 21 but also 20, 19,
// 18, and 17. Starting from a vector size of 16, the delta bit width becomes 4.
template <typename T>
constexpr auto k_packing_compute_deltas()
{
    // NOTE: the bit width for signed integral types does *not* include
    // the sign bit already.
    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= limits_max<::std::size_t>, "Overflow error.");

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
        // Current size of the coding vector.
        const auto cur_ncols = bit_width / cur_nbits;

        for (::std::size_t j = 0; j < cur_ncols; ++j) {
            // NOTE: do the number generation using the unsigned
            // counterpart.
            using uint_t = make_unsigned_t<T>;

            const auto rnd = rng.template random<uint_t>();

            // The generated random number will have the top two bits
            // set to 1. This ensures that the generated deltas don't vary
            // too much, while still retaining some randomness in the lower bits.
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
    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= limits_max<::std::size_t>, "Overflow error.");

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
// The component limits are computed from the deltas.
template <typename T>
constexpr auto k_packing_compute_limits()
{
    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= limits_max<::std::size_t>, "Overflow error.");

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
                // otherwise (i.e., odd delta).
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
// NOTE: contrary to the other functions above,
// here we return a table with a row for each available
// vector *size* (rather than delta bit width), starting from
// the largest possible one down to 2. The reason is that the
// same bit width might be associated to different vector sizes,
// thus producing different min/max encoded values.
template <typename T>
constexpr auto k_packing_compute_encoded_limits()
{
    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= limits_max<::std::size_t>, "Overflow error.");

    // Number of rows: from the max vector size (bit_width / 3u) down to 2.
    constexpr auto nrows = static_cast<::std::size_t>(bit_width / 3u - 2u + 1u);

    // Get the cvs and the components' limits.
    constexpr auto cvs = detail::k_packing_compute_cvs<T>();
    constexpr auto limits = detail::k_packing_compute_limits<T>();

    // Init the return value.
    auto retval = ::std::conditional_t<is_signed_v<T>, carray<carray<T, 2>, nrows>, carray<T, nrows>>{};

    // Compute the min/max encoded values for all possible
    // vector sizes.
    for (::std::size_t i = 0; i < nrows; ++i) {
        // The current vector size.
        const auto cur_size = bit_width / 3u - i;
        // The delta bit width corresponding to the current size.
        const auto cur_nbits = bit_width / cur_size;
        // The index into cvs/limits corresponding to
        // the delta bit width.
        const auto data_idx = cur_nbits - 3u;

        if constexpr (is_signed_v<T>) {
            T lim_min(0), lim_max(0);

            for (::std::size_t j = 0; j < cur_size; ++j) {
                lim_min += cvs[data_idx][j] * limits[data_idx][j][0];
                lim_max += cvs[data_idx][j] * limits[data_idx][j][1];
            }

            assert(lim_max > lim_min);

            retval[i][0] = lim_min;
            retval[i][1] = lim_max;
        } else {
            T lim(0);

            for (::std::size_t j = 0; j < cur_size; ++j) {
                lim += cvs[data_idx][j] * limits[data_idx][j];
            }

            retval[i] = lim;
        }
    }

    return retval;
}

// Construct a table for connecting a vector size to
// its corresponding delta bit width. This allows to
// avoid divisions at runtime in the packer/unpacker code.
template <typename T>
constexpr auto k_packing_compute_size_to_bits_table()
{
    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= limits_max<::std::size_t>, "Overflow error.");

    // Number of rows: from vector size 1 to the max size (bit_width / 3u).
    constexpr auto nrows = static_cast<::std::size_t>(bit_width / 3u);

    carray<unsigned, nrows> retval{};

    for (::std::size_t i = 0; i < nrows; ++i) {
        const auto cur_size = i + 1u;
        retval[i] = static_cast<unsigned>(bit_width / cur_size);
    }

    return retval;
}

// Package various data necessary for encoding/decoding in a compile-time tuple,
// so that we can take advantage of constexpr's UB checking (e.g., for signed
// overflows, out of bounds read writes, etc.).
template <typename T>
inline constexpr auto k_packing_data
    = ::std::make_tuple(detail::k_packing_compute_deltas<T>(), detail::k_packing_compute_cvs<T>(),
                        detail::k_packing_compute_limits<T>(), detail::k_packing_compute_encoded_limits<T>(),
                        detail::k_packing_compute_size_to_bits_table<T>());

// Small helper to compute the delta bit width corresponding to a vector size.
// Requires size in [1u, max_vector_size].
template <typename T>
constexpr unsigned k_packing_size_to_bits(unsigned size)
{
    assert(size > 0u);
    const auto idx = size - 1u;
    assert(idx < ::std::get<4>(k_packing_data<T>).size());
    return ::std::get<4>(k_packing_data<T>)[idx];
}

// Small helper to fetch the maximum vector size for the type T.
template <typename T>
constexpr auto k_packing_get_max_size()
{
    return ::std::get<4>(k_packing_data<T>).size();
}

// Return the component limits at index idx, given a type T and
// a delta bit width nbits.
template <typename T>
constexpr decltype(auto) k_packing_get_climits(unsigned nbits, unsigned idx)
{
    assert(nbits >= 3u);
    assert(nbits - 3u < ::std::get<2>(k_packing_data<T>).size());
    assert(idx < ::std::get<2>(k_packing_data<T>)[nbits - 3u].size());

    // NOTE: no component limit can ever be zero, if it is it means we
    // have something wrong in the indexing.
    if constexpr (is_signed_v<T>) {
        assert(::std::get<2>(k_packing_data<T>)[nbits - 3u][idx][0] != T(0));
        assert(::std::get<2>(k_packing_data<T>)[nbits - 3u][idx][1] != T(0));
    } else {
        assert(::std::get<2>(k_packing_data<T>)[nbits - 3u][idx] != T(0));
    }

    return ::std::get<2>(k_packing_data<T>)[nbits - 3u][idx];
}

// Return the component at index idx of the coding vector, given a type
// T and a delta bit width nbits.
template <typename T>
constexpr decltype(auto) k_packing_get_cvc(unsigned nbits, unsigned idx)
{
    assert(nbits >= 3u);
    assert(nbits - 3u < ::std::get<1>(k_packing_data<T>).size());
    assert(idx < ::std::get<1>(k_packing_data<T>)[nbits - 3u].size());
    // NOTE: no CV component can ever be zero, if it is it means we
    // have something wrong in the indexing.
    assert(::std::get<1>(k_packing_data<T>)[nbits - 3u][idx] != T(0));

    return ::std::get<1>(k_packing_data<T>)[nbits - 3u][idx];
}

// Return the encoded limits for a given vector size.
template <typename T>
constexpr decltype(auto) k_packing_get_elimits(unsigned size)
{
    assert(static_cast<unsigned>(limits_digits<T>) / 3u >= size);
    assert(static_cast<unsigned>(limits_digits<T>) / 3u - size < ::std::get<3>(k_packing_data<T>).size());

    return ::std::get<3>(k_packing_data<T>)[static_cast<unsigned>(limits_digits<T>) / 3u - size];
}

} // namespace detail

// Only allow certain integral types to be packable (this is due to the complications arising
// from integral promotion rules for short ints and char types).
template <typename T>
using is_k_packable = ::std::disjunction<::std::is_same<T, int>, ::std::is_same<T, unsigned>, ::std::is_same<T, long>,
                                         ::std::is_same<T, unsigned long>, ::std::is_same<T, long long>,
                                         ::std::is_same<T, unsigned long long>
#if defined(OBAKE_HAVE_GCC_INT128)
                                         ,
                                         ::std::is_same<T, __int128_t>, ::std::is_same<T, __uint128_t>
#endif
                                         >;

template <typename T>
inline constexpr bool is_k_packable_v = is_k_packable<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL KPackable = is_k_packable_v<T>;

#endif

// Kronecker packer.
#if defined(OBAKE_HAVE_CONCEPTS)
template <KPackable T>
#else
template <typename T, typename = ::std::enable_if_t<is_k_packable_v<T>>>
#endif
class k_packer
{
public:
    // Constructor from size.
    constexpr explicit k_packer(unsigned size) : m_value(0), m_index(0), m_size(size), m_nbits(0)
    {
        if (size) {
            // Get the delta bit width corresponding to the input size.
            if (obake_unlikely(size > detail::k_packing_get_max_size<T>())) {
                obake_throw(::std::overflow_error,
                            "Invalid size specified in the constructor of a Kronecker packer for the type '"
                                + ::obake::type_name<T>() + "': the maximum possible size is "
                                + detail::to_string(detail::k_packing_get_max_size<T>()) + ", but a size of "
                                + detail::to_string(size) + " was specified instead");
            }
            m_nbits = detail::k_packing_size_to_bits<T>(size);
        }
    }

    // Insert the next value into the packer.
    constexpr k_packer &operator<<(const T &n)
    {
        if (obake_unlikely(m_index == m_size)) {
            obake_throw(::std::out_of_range,
                        "Cannot push any more values to this Kronecker packer for the type '" + ::obake::type_name<T>()
                            + "': the number of "
                              "values already pushed to the packer is equal to the size used for construction ("
                            + detail::to_string(m_size) + ")");
        }

        // Special case for size 1: in that case, we don't
        // pack anything and just copy n into m_value.
        if (m_size == 1u) {
            m_value = n;
            ++m_index;

            return *this;
        }

        // Get the limits for the current component.
        const auto &lims = detail::k_packing_get_climits<T>(m_nbits, m_index);

        // Check that n is within the allowed limits for the current component.
        if constexpr (is_signed_v<T>) {
            if (obake_unlikely(n < lims[0] || n > lims[1])) {
                obake_throw(::std::overflow_error,
                            "Cannot push the value " + detail::to_string(n) + " to this Kronecker packer for the type '"
                                + ::obake::type_name<T>() + "': the value is outside the allowed range ["
                                + detail::to_string(lims[0]) + ", " + detail::to_string(lims[1]) + "]");
            }
        } else {
            if (obake_unlikely(n > lims)) {
                obake_throw(::std::overflow_error,
                            "Cannot push the value " + detail::to_string(n) + " to this Kronecker packer for the type '"
                                + ::obake::type_name<T>() + "': the value is outside the allowed range [0, "
                                + detail::to_string(lims) + "]");
            }
        }

        // Do the encoding.
        // NOTE: the coding vector might have excess components past m_index, we just
        // don't use them.
        const auto &c_value = detail::k_packing_get_cvc<T>(m_nbits, m_index);
        m_value += n * c_value;
        ++m_index;

        return *this;
    }

    // Fetch the encoded value.
    constexpr const T &get() const
    {
        // NOTE: if we pushed fewer values than m_size,
        // this will be equivalent to having pushed
        // zeroes for the missing values.
        return m_value;
    }

private:
    T m_value;
    unsigned m_index;
    unsigned m_size;
    unsigned m_nbits;
};

// Kronecker unpacker.
#if defined(OBAKE_HAVE_CONCEPTS)
template <KPackable T>
#else
template <typename T, typename = ::std::enable_if_t<is_k_packable_v<T>>>
#endif
class k_unpacker
{
public:
    constexpr explicit k_unpacker(const T &n, unsigned size) : m_value(n), m_index(0), m_size(size), m_nbits(0)
    {
        if (size) {
            // Get the delta bit width corresponding to the input size.
            if (obake_unlikely(size > detail::k_packing_get_max_size<T>())) {
                obake_throw(::std::overflow_error,
                            "Invalid size specified in the constructor of a Kronecker unpacker for the type '"
                                + ::obake::type_name<T>() + "': the maximum possible size is "
                                + detail::to_string(detail::k_packing_get_max_size<T>()) + ", but a size of "
                                + detail::to_string(size) + " was specified instead");
            }
            m_nbits = detail::k_packing_size_to_bits<T>(size);

            if (size > 1u) {
                // For a non-unitary size, check that the input encoded value is within the
                // allowed range.
                const auto &e_lim = detail::k_packing_get_elimits<T>(size);
                if constexpr (is_signed_v<T>) {
                    if (obake_unlikely(n < e_lim[0] || n > e_lim[1])) {
                        obake_throw(::std::overflow_error,
                                    "The value " + detail::to_string(n) + " passed to a Kronecker unpacker of size "
                                        + detail::to_string(size) + " is outside the allowed range ["
                                        + detail::to_string(e_lim[0]) + ", " + detail::to_string(e_lim[1]) + "]");
                    }
                } else {
                    if (obake_unlikely(n > e_lim)) {
                        obake_throw(::std::overflow_error,
                                    "The value " + detail::to_string(n) + " passed to a Kronecker unpacker of size "
                                        + detail::to_string(size) + " is outside the allowed range [0, "
                                        + detail::to_string(e_lim) + "]");
                    }
                }
            }
        } else {
            if (obake_unlikely(n)) {
                obake_throw(::std::invalid_argument, "Only a value of zero can be used in a Kronecker unpacker "
                                                     "with a size of zero, but a value of "
                                                         + detail::to_string(n) + " was provided instead");
            }
        }
    }

    k_unpacker &operator>>(T &n)
    {
        if (obake_unlikely(m_index == m_size)) {
            obake_throw(::std::out_of_range,
                        "Cannot unpack any more values from this Kronecker unpacker: the number of "
                        "values already unpacked is equal to the size used for construction ("
                            + detail::to_string(m_size) + ")");
        }

        // Special case for size 1: in that case, we don't
        // unpack anything and just copy m_value to n.
        if (m_size == 1u) {
            n = m_value;
            ++m_index;
            return *this;
        }

        // Get the index-th and index+1-th elements of the coding vector.
        // NOTE: the coding vector might have excess components past m_index+1, we just
        // don't use them.
        const auto &c_value0 = detail::k_packing_get_cvc<T>(m_nbits, m_index);
        const auto &c_value1 = detail::k_packing_get_cvc<T>(m_nbits, m_index + 1u);

        if constexpr (is_signed_v<T>) {
            // Extract the minimum encodable value and the index-th
            // lower component limit.
            const auto &e_min = detail::k_packing_get_elimits<T>(m_size)[0];
            const auto &c_min = detail::k_packing_get_climits<T>(m_nbits, m_index)[0];

            n = ((m_value - e_min) % c_value1) / c_value0 + c_min;
        } else {
            n = (m_value % c_value1) / c_value0;
        }

        ++m_index;

        return *this;
    }

private:
    T m_value;
    unsigned m_index;
    unsigned m_size;
    unsigned m_nbits;
};

} // namespace obake

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(pop)

#endif

#endif
