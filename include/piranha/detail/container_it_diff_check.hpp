// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_CONTAINER_IT_DIFF_CHECK_HPP
#define OBAKE_DETAIL_CONTAINER_IT_DIFF_CHECK_HPP

#include <stdexcept>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/exceptions.hpp>
#include <obake/type_traits.hpp>

namespace obake::detail
{

// Small helper to check that the difference type
// of the RA iterator of the (possibly const-qualified)
// input container can represent the container's size.
// This will ensure that we can use an iterator difference
// for indexing into c without overflow.
template <typename C>
inline void container_it_diff_check(C &c)
{
    // LCOV_EXCL_START
    using it_diff_t = decltype(c.end() - c.begin());
    using it_udiff_t = make_unsigned_t<it_diff_t>;
    if (obake_unlikely(c.size() > static_cast<it_udiff_t>(limits_max<it_diff_t>))) {
        obake_throw(::std::overflow_error,
                    "An overflow condition was detected: the size of a container is too large to be represented by "
                    "the difference type of the container's iterator");
    }
    // LCOV_EXCL_STOP
}

} // namespace obake::detail

#endif
