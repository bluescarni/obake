// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_TYPE_C_HPP
#define PIRANHA_DETAIL_TYPE_C_HPP

namespace piranha::detail
{

template <typename T>
struct type_c {
    using type = T;
};

} // namespace piranha::detail

#endif
