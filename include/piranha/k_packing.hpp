// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_K_PACKING_HPP
#define PIRANHA_K_PACKING_HPP

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/carray.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/detail/xoroshiro128_plus.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/utils/type_name.hpp>

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

template <typename T>
constexpr auto k_packing_compute_deltas()
{
    static_assert(is_integral_v<T>, "T must be an integral type.");

    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= ::std::get<1>(limits_minmax<::std::size_t>), "Overflow error.");

    constexpr auto nrows = static_cast<::std::size_t>(bit_width - 3u);
    constexpr auto ncols = static_cast<::std::size_t>(bit_width / 3u);

    carray<carray<T, ncols>, nrows> retval{};

    xoroshiro128_plus rng{12724899751400538854ull, 9282269007213506749ull};

    for (::std::size_t i = 0; i < nrows; ++i) {
        const auto cur_nbits = i + 3u;
        const auto cur_ncols = bit_width / cur_nbits;

        for (::std::size_t j = 0; j < cur_ncols; ++j) {
            using uint_t = make_unsigned_t<T>;

            const auto rnd = rng.template random<uint_t>();

            const auto hi = T(3) << (cur_nbits - 2u);
            const auto lo = static_cast<T>(rnd >> (static_cast<unsigned>(limits_digits<uint_t>) - cur_nbits + 2u));

            retval[i][j] = hi + lo;
        }
    }

    return retval;
}

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

template <typename T>
constexpr auto k_packing_signed_compute_minmaxes()
{
    static_assert(is_integral_v<T>, "T must be an integral type.");

    constexpr auto bit_width = static_cast<unsigned>(limits_digits<T>);
    static_assert(bit_width <= ::std::get<1>(limits_minmax<::std::size_t>), "Overflow error.");

    constexpr auto nrows = static_cast<::std::size_t>(bit_width - 3u);
    constexpr auto ncols = static_cast<::std::size_t>(bit_width / 3u);

    carray<carray<::std::pair<T, T>, ncols>, nrows> retval{};

    constexpr auto deltas = detail::k_packing_compute_deltas<T>();

    for (::std::size_t i = 0; i < nrows; ++i) {
        const auto cur_nbits = i + 3u;
        const auto cur_ncols = bit_width / cur_nbits;

        for (::std::size_t j = 0; j < cur_ncols; ++j) {
            const auto delta = deltas[i][j];

            if (delta % 2) {
                retval[i][j].first = (T(1) - delta) / T(2);
                retval[i][j].second = -retval[i][j].first;
            } else {
                retval[i][j].first = -delta / T(2);
                retval[i][j].second = delta / T(2) - T(1);
            }
        }
    }

    return retval;
}

template <typename T>
inline constexpr auto k_packing_unsigned_data
    = ::std::make_tuple(detail::k_packing_compute_deltas<T>(), detail::k_packing_compute_cvs<T>(),
                        detail::k_packing_signed_compute_minmaxes<T>());

template <typename T>
class unsigned_k_packer_impl
{
public:
    constexpr explicit unsigned_k_packer_impl(int nbits) : m_nbits(nbits)
    {
        if (piranha_unlikely(nbits < 3 || nbits > limits_digits<T>)) {
            piranha_throw(
                ::std::invalid_argument,
                "Invalid number of bits specified in the constructor of an unsigned Kronecker packer for the type '"
                    + ::piranha::type_name<T>()
                    + "': the number of bits must be at least 3 and not greater than the bit width of the integral "
                      "type ("
                    + ::std::to_string(limits_digits<T>) + "), but it is " + std::to_string(nbits) + " instead");
        }
    }

private:
    int m_nbits;
};

} // namespace detail

} // namespace piranha

#endif
