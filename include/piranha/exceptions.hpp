// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_EXCEPTIONS_HPP
#define PIRANHA_EXCEPTIONS_HPP

#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/type_traits.hpp>

#if defined(PIRANHA_WITH_STACK_TRACES)

#include <piranha/detail/stack_trace.hpp>

#endif

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
    // The decorating version of the call operator. This is preferred to the above wrt overload resolution
    // if there is at least one argument (the previous overload is "more" variadic), but it is disabled
    // if Str is not a string type or the construction of the decorated exception is not possible.
    template <typename Str, typename... Args,
              enable_if_t<conjunction<is_string_type<uncvref_t<Str>>,
                                      std::is_constructible<Exception, std::string, Args &&...>>::value,
                          int> = 0>
    [[noreturn]] void operator()(Str &&desc, Args &&... args) const
    {
        std::ostringstream oss;
#if defined(PIRANHA_WITH_BOOST_STACKTRACE)
        if (m_st) {
            stream_stacktrace(oss, *m_st);
        } else {
#endif
            // This is what is printed if stacktraces are not available/disabled.
            oss << "\nFunction name    : " << m_func;
            oss << "\nLocation         : " << m_file << ", line " << m_line;
#if defined(PIRANHA_WITH_BOOST_STACKTRACE)
        }
#endif
        oss << "\nException type   : " << piranha::demangle<Exception>();
        oss << "\nException message: " << std::forward<Str>(desc) << "\n";
        throw Exception(oss.str(), std::forward<Args>(args)...);
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
