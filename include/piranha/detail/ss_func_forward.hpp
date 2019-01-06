// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_SS_FUNC_FORWARD_HPP
#define PIRANHA_DETAIL_SS_FUNC_FORWARD_HPP

// Small macros to reduce typing when implementing single-statement
// function templates that perfectly forward their arguments.
// These are functions which look like this:
//
// template <typename T, typename U>
// constexpr auto func(T &&x, U &&y) noexcept(noexcept(body)) -> decltype(body)
// {
//    return body;
// }
//
// That is, func will call some other function in body while perfecty forwarding
// x and y, we want to make sure that func is constexpr/noexcept is the body
// is too and we want SFINAE to kick in to detect if body is malformed (hence the
// trailing return type). This pattern requires to type body 3 times, hence
// these macros.
//
// We need 2 macros, one for free functions and the other one for generic lambdas
// (which place the constexpr specifier differently).

#define PIRANHA_SS_FORWARD_LAMBDA(body)                                                                                \
    constexpr noexcept(noexcept(body))->decltype(body)                                                                 \
    {                                                                                                                  \
        return body;                                                                                                   \
    }

// NOTE: the bit with the useless namespace is to allow to use this macro with a closing semicolon
// (otherwise clang-format goes berserk).
#define PIRANHA_SS_FORWARD_FUNCTION(body)                                                                              \
    noexcept(noexcept(body))->decltype(body)                                                                           \
    {                                                                                                                  \
        return body;                                                                                                   \
    }                                                                                                                  \
    namespace _piranha_unused = ::piranha::_unused

namespace piranha::_unused
{
}

#endif
