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
#include <iostream>
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

// Stack trace data, in string form: file (including line number) and demangled function name.
// Put the alias in an anonymous namespace to prevent accidental ODR violations.
using stack_trace_data = ::std::vector<::std::array<::std::string, 2>>;

} // namespace

::std::string stack_trace_impl()
{
    // Prepare the stack trace data we will be writing into.
    stack_trace_data st_data;

    // Try to create the backtrace state.
    auto bt_state = ::backtrace_create_state(nullptr, 0, nullptr, nullptr);
    if (piranha_unlikely(!bt_state)) {
        ::std::cerr
            << "backtrace_create_state() could not allocate the state structure, returning an empty stacktrace.\n";
        ::std::cerr.flush();
        return "";
    }

    // Fetch the raw backtrace.
    const auto ret
        = ::backtrace_full(bt_state, 2, ::piranha_backtrace_callback, nullptr, static_cast<void *>(&st_data));
    if (piranha_unlikely(ret)) {
        ::std::cerr << "backtrace_full() failed with error code " << ret << ", returning an empty stacktrace.\n";
        ::std::cerr.flush();
        return "";
    }

    // Special case for an empty backtrace. Not sure if possible,
    // but checking it here simplifies the logic below.
    if (st_data.empty()) {
        return "";
    }

    // Create a vector of string indices over st_data.
    ::std::vector<::std::string> v_indices;
    for (decltype(st_data.size()) i = 0; i < st_data.size(); ++i) {
        v_indices.push_back(::std::to_string(i));
    }

    // Reverse st_data and the indices vector.
    ::std::reverse(st_data.begin(), st_data.end());
    ::std::reverse(v_indices.begin(), v_indices.end());

    // Get the max widths of the first 2 table columns
    const auto max_idx_w = ::std::max_element(v_indices.begin(), v_indices.end(),
                                              [](const auto &s1, const auto &s2) { return s1.size() < s2.size(); })
                               ->size();
    const auto max_file_name_w = (*::std::max_element(
        st_data.begin(), st_data.end(), [](const auto &a1, const auto &a2) { return a1[0].size() < a2[0].size(); }))[0]
                                     .size();

    // Produce the formatted table.
    ::std::string retval;
    for (decltype(st_data.size()) i = 0; i < st_data.size(); ++i) {
        retval += "# " + ::std::string(max_idx_w - v_indices[i].size(), ' ') + v_indices[i] + " | " + st_data[i][0]
                  + ::std::string(max_file_name_w - st_data[i][0].size(), ' ') + " | " + st_data[i][1];
        if (i != st_data.size() - 1u) {
            retval += '\n';
        }
    }

    return retval;
}

} // namespace piranha::detail

int piranha_backtrace_callback(void *data, ::std::uintptr_t, const char *filename, int lineno, const char *funcname)
{
    try {
        auto &st_data = *static_cast<::piranha::detail::stack_trace_data *>(data);

        auto file_name = ::std::string(filename ? filename : "<unknown file>") + ":" + ::std::to_string(lineno);
        auto func_name = funcname ? ::piranha::detail::demangle_impl(funcname) : "<unknown function>";

        st_data.push_back(::std::array{::std::move(file_name), ::std::move(func_name)});
    } catch (...) {
        return -1;
    }
    return 0;
}
