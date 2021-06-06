// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_EXCEPTIONS_HPP
#define OBAKE_EXCEPTIONS_HPP

#include <string>
#include <type_traits>
#include <utility>

#include <obake/stack_trace.hpp>
#include <obake/type_name.hpp>
#include <obake/type_traits.hpp>

namespace obake::detail
{

template <typename Exception>
struct ex_thrower {
    // Determine the type of the __LINE__ macro.
    using line_type = remove_cvref_t<decltype(__LINE__)>;
    // The non-decorating version of the call operator.
    template <typename... Args, ::std::enable_if_t<::std::is_constructible_v<Exception, Args...>, int> = 0>
    [[noreturn]] void operator()(Args &&... args) const
    {
        throw Exception(::std::forward<Args>(args)...);
    }
    // The decorating version of the call operator. This is a better match during overload resolution
    // if there is at least one argument (the previous overload is "more" variadic), but it is disabled
    // if Str is not a string-like type or the construction of the decorated exception is not possible.
    template <typename Str, typename... Args,
              ::std::enable_if_t<::std::conjunction_v<is_string_like<::std::remove_reference_t<Str>>,
                                                      ::std::is_constructible<Exception, ::std::string, Args...>>,
                                 int> = 0>
    [[noreturn]] void operator()(Str &&desc, Args &&... args) const
    {
        ::std::string str = ::obake::stack_trace(1) + '\n';

        str += "\nException type   : ";
        str += ::obake::type_name<Exception>();
        str += "\nException message: ";
        str += ::std::forward<Str>(desc);
        str += '\n';

        throw Exception(::std::move(str), ::std::forward<Args>(args)...);
    }
    const char *m_file;
    const line_type m_line;
    const char *m_func;
};

} // namespace obake::detail

// Exception throwing macro.
#define obake_throw(exception_type, ...)                                                                               \
    (::obake::detail::ex_thrower<exception_type>{__FILE__, __LINE__, __func__}(__VA_ARGS__))

#endif
