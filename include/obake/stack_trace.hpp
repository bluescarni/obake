// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_STACK_TRACE_HPP
#define OBAKE_STACK_TRACE_HPP

#include <atomic>
#include <string>

#include <obake/config.hpp>
#include <obake/detail/visibility.hpp>

namespace obake
{

namespace detail
{

OBAKE_DLL_PUBLIC extern ::std::atomic_bool stack_trace_enabled;

OBAKE_DLL_PUBLIC ::std::string stack_trace_impl(unsigned);

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct stack_trace_enabled_msvc {
    bool operator()() const
    {
        return detail::stack_trace_enabled.load(::std::memory_order_relaxed);
    }
};

struct set_stack_trace_enabled_msvc {
    void operator()(bool status) const
    {
        detail::stack_trace_enabled.store(status, ::std::memory_order_relaxed);
    }
};

inline constexpr auto stack_trace_enabled = stack_trace_enabled_msvc{};
inline constexpr auto set_stack_trace_enabled = set_stack_trace_enabled_msvc{};

struct stack_trace_msvc {
    auto operator()(unsigned skip = 0) const
    {
        if (::obake::stack_trace_enabled()) {
            return detail::stack_trace_impl(skip);
        }
        return ::std::string{"<Stack trace generation has been disabled at runtime>"};
    }
};

inline constexpr auto stack_trace = stack_trace_msvc{};

#else

// Test/set whether stack trace generation is enabled at runtime.
inline constexpr auto stack_trace_enabled
    = []() { return detail::stack_trace_enabled.load(::std::memory_order_relaxed); };
inline constexpr auto set_stack_trace_enabled
    = [](bool status) { detail::stack_trace_enabled.store(status, ::std::memory_order_relaxed); };

// Generate a stack trace starting from the call site of this function.
// The 'skip' parameter indicates how many stack levels should be skipped
// (from bottom to top).
inline constexpr auto stack_trace = [](unsigned skip = 0) {
    if (::obake::stack_trace_enabled()) {
        return detail::stack_trace_impl(skip);
    }
    return ::std::string{"<Stack trace generation has been disabled at runtime>"};
};

#endif

} // namespace obake

#endif
