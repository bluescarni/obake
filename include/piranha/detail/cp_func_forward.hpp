// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_CP_FUNC_FORWARD_HPP
#define PIRANHA_DETAIL_CP_FUNC_FORWARD_HPP

// Small macro to reduce typing when implementing the cp functions.
// It will take care of marking the function constexpr, noexcept if possible,
// setup the trailing return type for SFINAE and implement the function's body.
#define PIRANHA_CP_FUNC_FORWARD(body)                                                                                  \
    constexpr noexcept(noexcept(body))->decltype(body)                                                                 \
    {                                                                                                                  \
        return body;                                                                                                   \
    }

#endif
