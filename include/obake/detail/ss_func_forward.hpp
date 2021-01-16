// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_SS_FUNC_FORWARD_HPP
#define OBAKE_DETAIL_SS_FUNC_FORWARD_HPP

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
// x and y, we want to make sure that func is constexpr/noexcept if the body
// is too, and we want SFINAE to kick in to detect if body is malformed (hence the
// trailing return type). This pattern requires to type body 3 times, hence
// these macros.
//
// We need 3 macros, one for free functions, one for generic lambdas
// (which place the constexpr specifier differently) and one for
// member functions.
#define OBAKE_SS_FORWARD_LAMBDA(body)                                                                                  \
    constexpr noexcept(noexcept(body))->decltype(body)                                                                 \
    {                                                                                                                  \
        return body;                                                                                                   \
    }

// NOTE: the bit with the useless namespace is to allow to use this macro with a closing semicolon
// (otherwise clang-format goes berserk).
#define OBAKE_SS_FORWARD_FUNCTION(body)                                                                                \
    noexcept(noexcept(body))->decltype(body)                                                                           \
    {                                                                                                                  \
        return body;                                                                                                   \
    }                                                                                                                  \
    namespace _obake_unused = ::obake::_unused

#define OBAKE_SS_FORWARD_MEMBER_FUNCTION(body)                                                                         \
    noexcept(noexcept(body))->decltype(body)                                                                           \
    {                                                                                                                  \
        return body;                                                                                                   \
    }

namespace obake::_unused
{
}

#endif
