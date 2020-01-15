// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)

// NOTE: setting the backend explicitly is needed for
// proper support for clang-cl.
#define BOOST_STACKTRACE_USE_WINDBG
#include <boost/stacktrace.hpp>
#undef BOOST_STACKTRACE_USE_WINDBG

#elif defined(__APPLE__)

// This definition is needed for building on OSX.
#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace.hpp>
#undef BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED

#else

// Default case, no special definitions required.
#include <boost/stacktrace.hpp>

#endif

#include <obake/config.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/stack_trace.hpp>

#if defined(__clang__)

// Silence clang warning when doing overflow checking below.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"

#endif

namespace obake::detail
{

::std::string stack_trace_impl(unsigned skip)
{
    // The fixed number of frames we need to skip in order
    // to generate the stack trace from the point of invocation
    // of the top level function. It *should* be 2, but on Windows
    // we need 4 for some reason. Not sure if this is a Boost
    // issue or not.
    constexpr unsigned fixed_skip =
#if defined(_WIN32)
        4u
#else
        2u
#endif
        ;

    // Check the skip parameter.
    // LCOV_EXCL_START
    if (obake_unlikely(skip > ::std::numeric_limits<::std::size_t>::max() - fixed_skip)) {
        return "The stack trace could not be generated due to an overflow condition.";
    }
    // LCOV_EXCL_STOP

    // Generate the stack trace.
    const auto tot_skip = static_cast<::std::size_t>(fixed_skip + static_cast<::std::size_t>(skip));
    ::boost::stacktrace::stacktrace st(tot_skip, ::std::numeric_limits<::std::size_t>::max() - tot_skip);

    // Special case an empty backtrace.
    if (!st.size()) {
        return "";
    }

    // Generate the first two columns of the table, and compute
    // the max column widths.
    ::std::string::size_type max_idx_width = 0, max_fname_width = 0;
    ::std::vector<::std::array<::std::string, 2>> indices_fnames;
    indices_fnames.reserve(static_cast<decltype(indices_fnames.size())>(st.size()));
    for (decltype(st.size()) i = 0; i < st.size(); ++i) {
        indices_fnames.push_back(::std::array<::std::string, 2>{
            {detail::to_string(i), st[i].source_file() + ":" + detail::to_string(st[i].source_line())}});
        max_idx_width = ::std::max(max_idx_width, indices_fnames.back()[0].size());
        max_fname_width = ::std::max(max_fname_width, indices_fnames.back()[1].size());
    }

    // Assemble the formatted table.
    ::std::string retval;
    auto it_indices_fnames = indices_fnames.crbegin();
    for (auto it = st.crbegin(); it != st.crend(); ++it, ++it_indices_fnames) {
        auto it_name = it->name();

#if defined(_WIN32)
        // NOTE: earlier versions of Boost stacktrace might produce names with trailing
        // null chars on Windows with the DbgEng backend:
        // https://github.com/boostorg/stacktrace/commit/36734b15319902de1dbfe287bc229141df3c04b8
        // In the future, we should put a version check here.
        while (!it_name.empty() && it_name.back() == '\0') {
            it_name.pop_back();
        }
#endif

        retval += "# " + ::std::string(max_idx_width - (*it_indices_fnames)[0].size(), ' ') + (*it_indices_fnames)[0]
                  + " | " + (*it_indices_fnames)[1]
                  + ::std::string(max_fname_width - (*it_indices_fnames)[1].size(), ' ') + " | " + ::std::move(it_name);
        if (it != st.crend() - 1) {
            retval += '\n';
        }
    }

    return retval;
}

} // namespace obake::detail

#if defined(__clang__)

#pragma clang diagnostic pop

#endif
