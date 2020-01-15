// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_TYPE_C_HPP
#define OBAKE_DETAIL_TYPE_C_HPP

namespace obake::detail
{

// A small wrapper to represent types
// in constexpr functions.
template <typename T>
struct type_c {
    using type = T;
};

} // namespace obake::detail

#endif
