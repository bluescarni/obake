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
#include <piranha/detail/to_string.hpp>
#include <piranha/utils/demangle.hpp>
#include <piranha/utils/stack_trace.hpp>

namespace piranha::detail
{

// NOTE: put local names in an anonymous namespace, in order to pre-empt
// potential ODR violations.
namespace
{

// Stack trace data, in string form: level, file (including line number) and demangled function name.
using stack_trace_data = ::std::vector<::std::array<::std::string, 3>>;

// The callback for the main backtrace function. Needs to have C linkage.
extern "C" {
static int backtrace_callback(void *data, ::std::uintptr_t, const char *filename, int lineno, const char *funcname)
{
    // Catch any exception that might be thrown, as this function
    // will be called from a C library and throwing in such case is
    // undefined behaviour.
    try {
        auto &st_data = *static_cast<stack_trace_data *>(data);

        auto file_name = ::std::string(filename ? filename : "<unknown file>") + ":" + detail::to_string(lineno);
        auto func_name = funcname ? demangle_impl(funcname) : "<unknown function>";

        // NOTE: the level is left empty, it will be filled in later.
        st_data.push_back(::std::array{::std::string{}, ::std::move(file_name), ::std::move(func_name)});
    } catch (...) {
        return -1;
    }
    return 0;
}
}

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
    // NOTE: apparently this can be created once and then re-used in future invocations.
    // This also drastically reduces memory usage and increases runtime performance.
    // We create this as a thread_local static so that we can call backtrace_create_state()
    // in single-threaded mode. See also:
    // https://github.com/boostorg/stacktrace/commit/4123beb4af6ff4e36769905b87c206da39190847
    thread_local auto bt_state = ::backtrace_create_state(nullptr, 0, nullptr, nullptr);
    if (piranha_unlikely(!bt_state)) {
        return "The stack trace could not be generated because the backtrace_create_state() function failed to "
               "allocate the state structure.";
    }

    // Fetch the raw backtrace.
    const auto ret = ::backtrace_full(bt_state, 2 + static_cast<int>(skip), backtrace_callback, nullptr,
                                      static_cast<void *>(&st_data));
    if (piranha_unlikely(ret)) {
        return "The stack trace could not be generated because the backtrace_full() function returned the error code "
               + detail::to_string(ret) + ".";
    }

    // Special case for an empty backtrace. This can happen, e.g., if the value of
    // 'skip' is large enough.
    if (st_data.empty()) {
        return ::std::string{};
    }

    // Add manually the levels, and get the max widths of the first 2 table columns.
    const auto [max_idx_w, max_file_name_w] = [&st_data]() {
        ::std::array<::std::string::size_type, 2> retval{0, 0};
        for (decltype(st_data.size()) i = 0; i < st_data.size(); ++i) {
            st_data[i][0] = detail::to_string(i);
            retval[0] = ::std::max(st_data[i][0].size(), retval[0]);
            retval[1] = ::std::max(st_data[i][1].size(), retval[1]);
        }
        return retval;
    }();

    // Produce the formatted table, iterating on st_data in reverse order.
    ::std::string retval;
    const auto r_end = st_data.crend();
    for (auto r_it = st_data.crbegin(); r_it != r_end; ++r_it) {
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
