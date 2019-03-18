// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <backtrace-supported.h>
#include <backtrace.h>

#if !defined(BACKTRACE_SUPPORTED)

#error The backtrace-supported.h header is reporting that libbacktrace is not supported.

#endif

#include <piranha/config.hpp>
#include <piranha/utils/demangle.hpp>
#include <piranha/utils/stack_trace.hpp>

// The callback for the main backtrace function. Needs to have C linkage.
extern "C" {
int piranha_backtrace_callback(void *, ::std::uintptr_t, const char *, int, const char *);
}

namespace piranha::detail
{

namespace
{

// Stack trace data, in string form: level, file (including line number) and demangled function name.
// Put the alias in an anonymous namespace to prevent accidental ODR violations.
using stack_trace_data = ::std::vector<::std::array<::std::string, 3>>;

} // namespace

::std::string stack_trace_impl(unsigned skip)
{
    // Check the skip parameter.
    if (piranha_unlikely(skip > static_cast<unsigned>(::std::numeric_limits<int>::max()) - 2u)) {
        return "The stack trace could not be generated due to an overflow condition.";
    }

    // Prepare the stack trace data we will be writing into.
    stack_trace_data st_data;

    // Try to create the backtrace state.
    auto bt_state = ::backtrace_create_state(nullptr, 0, nullptr, nullptr);
    if (piranha_unlikely(!bt_state)) {
        return "The stack trace could not be generated because the backtrace_create_state() function failed to "
               "allocate the state structure.";
    }

    // Fetch the raw backtrace.
    const auto ret = ::backtrace_full(bt_state, 2 + static_cast<int>(skip), ::piranha_backtrace_callback, nullptr,
                                      static_cast<void *>(&st_data));
    if (piranha_unlikely(ret)) {
        return "The stack trace could not be generated because the backtrace_full() function returned the error code "
               + std::to_string(ret) + ".";
    }

    // Special case for an empty backtrace. This can happen, e.g., if the value of
    // 'skip' is large enough.
    if (st_data.empty()) {
        return ::std::string{};
    }

    // Add manually the levels.
    for (decltype(st_data.size()) i = 0; i < st_data.size(); ++i) {
        st_data[i][0] = ::std::to_string(i);
    }

    // Get the max widths of the first 2 table columns
    const auto max_idx_w = (*::std::max_element(
        st_data.begin(), st_data.end(), [](const auto &a1, const auto &a2) { return a1[0].size() < a2[0].size(); }))[0]
                               .size();
    const auto max_file_name_w = (*::std::max_element(
        st_data.begin(), st_data.end(), [](const auto &a1, const auto &a2) { return a1[1].size() < a2[1].size(); }))[1]
                                     .size();

    // Produce the formatted table, iterating on st_data in reverse order.
    ::std::string retval;
    const auto r_end = ::std::make_reverse_iterator(st_data.cbegin());
    for (auto r_it = ::std::make_reverse_iterator(st_data.cend()); r_it != r_end; ++r_it) {
        const auto &a = *r_it;
        retval += "# " + ::std::string(max_idx_w - a[0].size(), ' ') + a[0] + " | " + a[1]
                  + ::std::string(max_file_name_w - a[1].size(), ' ') + " | " + a[2];
        if (r_it != r_end - 1) {
            retval += '\n';
        }
    }

    return retval;
}

} // namespace piranha::detail

int piranha_backtrace_callback(void *data, ::std::uintptr_t, const char *filename, int lineno, const char *funcname)
{
    // Catch any exception that might be thrown, as this function
    // will be called from a C library and throwing in such case is
    // undefined behaviour.
    try {
        auto &st_data = *static_cast<::piranha::detail::stack_trace_data *>(data);

        auto file_name = ::std::string(filename ? filename : "<unknown file>") + ":" + ::std::to_string(lineno);
        auto func_name = funcname ? ::piranha::detail::demangle_impl(funcname) : "<unknown function>";

        // NOTE: the level is left empty, it will be filled in later.
        st_data.push_back(::std::array{::std::string{}, ::std::move(file_name), ::std::move(func_name)});
    } catch (...) {
        return -1;
    }
    return 0;
}
