// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_TO_STRING_HPP
#define PIRANHA_DETAIL_TO_STRING_HPP

#include <string>

#include <piranha/config.hpp>
#include <piranha/detail/visibility.hpp>

namespace piranha::detail
{

template <typename T>
inline ::std::string to_string(const T &x)
{
    return ::std::to_string(x);
}

// NOTE: provide specialised implementations for
// common integral types. These implementations
// ensure that the string representations are
// locale-agnostic.

// Char types.
template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const char &);

template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const signed char &);

template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const unsigned char &);

// Shorts.
template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const short &);

template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const unsigned short &);

// Ints.
template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const int &);

template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const unsigned &);

// Longs.
template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const long &);

template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const unsigned long &);

// Longlongs.
template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const long long &);

template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const unsigned long long &);

#if defined(PIRANHA_HAVE_GCC_INT128)

// 128bit ints.
template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const __uint128_t &);

template <>
PIRANHA_DLL_PUBLIC ::std::string to_string(const __int128_t &);

#endif

} // namespace piranha::detail

#endif
