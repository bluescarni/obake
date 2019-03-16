// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_UTILS_STACK_TRACE_HPP
#define PIRANHA_UTILS_STACK_TRACE_HPP

#include <piranha/config.hpp>

#if !defined(PIRANHA_WITH_STACK_TRACES)

#error The utils/stack_traces.hpp header was included, but piranha was not configured with support for stack traces.

#endif

#include <string>

#include <piranha/detail/visibility.hpp>

namespace piranha
{

namespace detail
{

PIRANHA_PUBLIC ::std::string stack_trace_impl();

}

inline constexpr auto stack_trace = []() { return detail::stack_trace_impl(); };
} // namespace piranha

#endif
