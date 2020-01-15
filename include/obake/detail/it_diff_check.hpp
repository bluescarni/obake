// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_IT_DIFF_CHECK_HPP
#define OBAKE_DETAIL_IT_DIFF_CHECK_HPP

#include <iterator>
#include <stdexcept>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/exceptions.hpp>
#include <obake/type_name.hpp>
#include <obake/type_traits.hpp>

namespace obake::detail
{

// Small helper to check that the difference type
// of It is able to represent an unsigned size.
// Requires It to be an iterator, T an unsigned integral type.
template <typename It, typename T>
inline void it_diff_check(const T &size)
{
    static_assert(is_integral_v<T> && !is_signed_v<T>);

    // LCOV_EXCL_START
    using it_diff_t = typename ::std::iterator_traits<It>::difference_type;
    using it_udiff_t = make_unsigned_t<it_diff_t>;
    if (obake_unlikely(size > static_cast<it_udiff_t>(limits_max<it_diff_t>))) {
        obake_throw(::std::overflow_error,
                    "An overflow condition was detected: the difference type of the iterator type '"
                        + ::obake::type_name<It>() + "' cannot represent a size of " + detail::to_string(size));
    }
    // LCOV_EXCL_STOP
}

// Small convenience wrapper to run it_diff_check()
// on a container type (i.e., this will check that
// the difference type of the container's iterator type
// can represent the container's size).
template <typename C>
inline void container_it_diff_check(C &c)
{
    detail::it_diff_check<decltype(c.begin())>(c.size());
}

} // namespace obake::detail

#endif
