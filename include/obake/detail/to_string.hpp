// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_TO_STRING_HPP
#define OBAKE_DETAIL_TO_STRING_HPP

#include <string>

#include <obake/config.hpp>
#include <obake/detail/visibility.hpp>

namespace obake::detail
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
OBAKE_DLL_PUBLIC ::std::string to_string(const char &);

template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const signed char &);

template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const unsigned char &);

// Shorts.
template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const short &);

template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const unsigned short &);

// Ints.
template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const int &);

template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const unsigned &);

// Longs.
template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const long &);

template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const unsigned long &);

// Longlongs.
template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const long long &);

template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const unsigned long long &);

#if defined(OBAKE_HAVE_GCC_INT128)

// 128bit ints.
template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const __uint128_t &);

template <>
OBAKE_DLL_PUBLIC ::std::string to_string(const __int128_t &);

#endif

} // namespace obake::detail

#endif
