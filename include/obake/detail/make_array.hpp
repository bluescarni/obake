// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_MAKE_ARRAY_HPP
#define OBAKE_DETAIL_MAKE_ARRAY_HPP

#include <array>
#include <utility>

namespace obake::detail
{

// NOTE: a small utility to create a std::array from a variadic
// pack. This is useful because it looks like in current GCC versions
// there might a bug that lead to memory leaks when one does:
//
// return std::array{func(args)...};
//
// and func() throws an exception. By writing instead
//
// return make_array(func(args)...);
//
// the compiler is forced to evaluate all func(args) before
// actually constructing the array, and this seems to avoid
// the leak. See:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66139
template <typename... Args>
inline auto make_array(Args &&...args)
{
    return ::std::array{::std::forward<Args>(args)...};
}

} // namespace obake::detail

#endif
