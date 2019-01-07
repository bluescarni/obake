// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_PRIORITY_TAG_HPP
#define PIRANHA_DETAIL_PRIORITY_TAG_HPP

#include <cstddef>

namespace piranha::detail
{

template <::std::size_t I>
struct priority_tag : ::piranha::detail::priority_tag<I - 1u> {
};

template <>
struct priority_tag<0> {
};

} // namespace piranha::detail

#endif
