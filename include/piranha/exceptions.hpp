// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_EXCEPTIONS_HPP
#define PIRANHA_EXCEPTIONS_HPP

#include <string>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/type_traits.hpp>
#include <piranha/utils/stack_trace.hpp>
#include <piranha/utils/type_name.hpp>

namespace piranha::detail
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
        ::std::string str =
#if defined(PIRANHA_WITH_STACK_TRACES)
            ::piranha::stack_trace(1) + '\n'
#else
            ::std::string("Function name    : ") + m_func + "\nLocation         : " + m_file + ", line "
            + detail::to_string(m_line)
#endif
            ;

        str += "\nException type   : ";
        str += ::piranha::type_name<Exception>();
        str += "\nException message: ";
        str += ::std::forward<Str>(desc);
        str += '\n';

        throw Exception(::std::move(str), ::std::forward<Args>(args)...);
    }
    const char *m_file;
    const line_type m_line;
    const char *m_func;
};

} // namespace piranha::detail

// Exception throwing macro.
#define piranha_throw(exception_type, ...)                                                                             \
    (::piranha::detail::ex_thrower<exception_type>{__FILE__, __LINE__, __func__}(__VA_ARGS__))

#endif
