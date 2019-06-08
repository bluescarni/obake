// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_CARRAY_HPP
#define PIRANHA_DETAIL_CARRAY_HPP

#include <array>
#include <cassert>
#include <cstddef>

namespace piranha::detail
{

#if defined(_MSC_VER) && _MSC_VER < 1910

// NOTE: a minimal constexpr-enabled std::array
// implementation for use in MSVC 2015 and earlier.

template <typename T, ::std::size_t N>
struct carray {
    constexpr carray() : m_value{} {}
    constexpr T &operator[](::std::size_t n)
    {
        assert(n < N);
        return m_value[n];
    }
    constexpr const T &operator[](::std::size_t n) const
    {
        assert(n < N);
        return m_value[n];
    }
    constexpr ::std::size_t size() const
    {
        return N;
    }
    // NOTE: supply get() functions
    // for use in structured bindings.
    template <::std::size_t M>
    friend constexpr T &get(carray &a)
    {
        return a[M];
    }
    template <::std::size_t M>
    friend constexpr const T &get(const carray &a)
    {
        return a[M];
    }
    T m_value[N];
};

#else

template <typename T, ::std::size_t N>
using carray = ::std::array<T, N>;

#endif

} // namespace piranha::detail

#if defined(_MSC_VER) && _MSC_VER < 1910

// NOTE: specialise a couple of std classes
// in order to support structured bindings.
#include <tuple>
#include <type_traits>

namespace std
{

template <typename T, size_t N>
struct tuple_size<::piranha::detail::carray<T, N>> : integral_constant<size_t, N> {
};

template <size_t I, typename T, size_t N>
struct tuple_element<I, ::piranha::detail::carray<T, N>> {
    using type = T;
};

} // namespace std

#endif

#endif
