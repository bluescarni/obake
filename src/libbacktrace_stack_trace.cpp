// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
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

#if __has_include(<cxxabi.h>)

#include <cstdlib>
#include <cxxabi.h>
#include <memory>

#endif

#include <backtrace-supported.h>
#include <backtrace.h>

#if !defined(BACKTRACE_SUPPORTED)

#error The backtrace-supported.h header is reporting that libbacktrace is not supported.

#endif

#include <obake/config.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/stack_trace.hpp>

namespace obake::detail
{

// NOTE: put local names in an anonymous namespace, in order to pre-empt
// potential ODR violations.
namespace
{

::std::string demangle_impl(const char *s)
{
#if __has_include(<cxxabi.h>)
    // NOTE: wrap std::free() in a local lambda, so we avoid
    // potential ambiguities when taking the address of std::free().
    // See:
    // https://stackoverflow.com/questions/27440953/stdunique-ptr-for-c-functions-that-need-free
    auto deleter = [](void *ptr) { ::std::free(ptr); };

    // NOTE: abi::__cxa_demangle will return a pointer allocated by std::malloc, which we will delete via std::free().
    ::std::unique_ptr<char, decltype(deleter)> res{::abi::__cxa_demangle(s, nullptr, nullptr, nullptr), deleter};

    // NOTE: return the original string if demangling fails.
    return res ? ::std::string(res.get()) : ::std::string(s);
#else
    return ::std::string(s);
#endif
}

// Stack trace data, in string form: level, file (including line number) and demangled function name.
using stack_trace_data = ::std::vector<::std::array<::std::string, 3>>;

// The callback for the main backtrace function. Needs to have C linkage.
extern "C" {
int backtrace_callback(void *data, ::std::uintptr_t, const char *filename, int lineno, const char *funcname)
{
    // Catch any exception that might be thrown, as this function
    // will be called from a C library and throwing in such case is
    // undefined behaviour.
    try {
        auto &st_data = *static_cast<stack_trace_data *>(data);

        auto file_name = ::std::string(filename ? filename : "<unknown file>") + ":" + detail::to_string(lineno);
        auto func_name = funcname ? detail::demangle_impl(funcname) : "<unknown function>";

        // NOTE: the level is left empty, it will be filled in later.
        st_data.push_back(
            ::std::array<::std::string, 3>{::std::string{}, ::std::move(file_name), ::std::move(func_name)});
        // LCOV_EXCL_START
    } catch (...) {
        return -1;
    }
    // LCOV_EXCL_STOP
    return 0;
}
}

} // namespace

::std::string stack_trace_impl(unsigned skip)
{
    // Check the skip parameter.
    // LCOV_EXCL_START
    if (obake_unlikely(skip > static_cast<unsigned>(::std::numeric_limits<int>::max()) - 2u)) {
        return "The stack trace could not be generated due to an overflow condition.";
    }
    // LCOV_EXCL_STOP

    // Prepare the stack trace data we will be writing into.
    stack_trace_data st_data;

    // Try to create the backtrace state.
    // NOTE: apparently this can be created once and then re-used in future invocations.
    // This also drastically reduces memory usage and increases runtime performance.
    // We create this as a thread_local static so that we can call backtrace_create_state()
    // in single-threaded mode. See also:
    // https://github.com/boostorg/stacktrace/commit/4123beb4af6ff4e36769905b87c206da39190847
    // NOTE: the value returned by backtrace_create_state() needs not to be freed, according
    // to the docs in the libbacktrace headers. Thus, it *should* be safe to use
    // also on MinGW's buggy thread_local implementation.
    thread_local auto bt_state = ::backtrace_create_state(nullptr, 0, nullptr, nullptr);
    if (obake_unlikely(!bt_state)) {
        // LCOV_EXCL_START
        return "The stack trace could not be generated because the backtrace_create_state() function failed to "
               "allocate the state structure.";
        // LCOV_EXCL_STOP
    }

    // Fetch the raw backtrace.
    const auto ret = ::backtrace_full(bt_state, 2 + static_cast<int>(skip), backtrace_callback, nullptr,
                                      static_cast<void *>(&st_data));
    if (obake_unlikely(ret)) {
        // LCOV_EXCL_START
        return "The stack trace could not be generated because the backtrace_full() function returned the error code "
               + detail::to_string(ret) + ".";
        // LCOV_EXCL_STOP
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

} // namespace obake::detail
