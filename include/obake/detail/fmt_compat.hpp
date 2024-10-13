// Copyright 2020, 2021, 2022 Francesco Biscani (bluescarni@gmail.com), Dario Izzo (dario.izzo@gmail.com)
//
// This file is part of the heyoka library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_FMT_COMPAT_HPP
#define OBAKE_DETAIL_FMT_COMPAT_HPP

#include <sstream>
#include <stdexcept>

#include <fmt/core.h>
#include <fmt/format.h>

namespace obake::detail
{

struct ostream_formatter {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        if (ctx.begin() != ctx.end()) {
            // LCOV_EXCL_START
            throw std::invalid_argument("The ostream formatter does not accept any format string");
            // LCOV_EXCL_STOP
        }

        return ctx.begin();
    }

    template <typename T, typename FormatContext>
    auto format(const T &x, FormatContext &ctx) const
    {
        std::ostringstream oss;
        oss << x;

        return fmt::format_to(ctx.out(), "{}", oss.str());
    }
};

} // namespace obake::detail

#endif
