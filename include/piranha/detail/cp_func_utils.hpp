// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_CP_FUNC_UTILS_HPP
#define PIRANHA_DETAIL_CP_FUNC_UTILS_HPP

// Small macros to reduce typing when implementing the cp functions.

// Helper for the implementation of the lambda function objects implementing
// the customisable functions. It will take care of declaring the call operator
// of the lambda function constexpr, mark it conditionally noexcept, and implement
// it with trailing return type for SFINAE.
#define PIRANHA_IMPLEMENT_CP_LAMBDA(body)                                                                              \
    constexpr noexcept(noexcept(body))->decltype(body)                                                                 \
    {                                                                                                                  \
        return body;                                                                                                   \
    }

namespace piranha::_unused
{
}

// Similar to the above, but for a free function rather than a lambda.
// The namespace bits at the end are to be able to put a ';' after using
// this macro (otherwise clang-format will do weird stuff).
#define PIRANHA_IMPLEMENT_CP_FUNC(body)                                                                                \
    noexcept(noexcept(body))->decltype(body)                                                                           \
    {                                                                                                                  \
        return body;                                                                                                   \
    }                                                                                                                  \
    namespace _piranha_unused = ::piranha::_unused

#endif
