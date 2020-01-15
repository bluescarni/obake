// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_MPPP_UTILS_HPP
#define OBAKE_DETAIL_MPPP_UTILS_HPP

#include <cstddef>
#include <type_traits>

#include <mp++/integer.hpp>

namespace obake::detail
{

// Small helper to detect if a type is an
// mppp::integer.
template <typename>
struct is_mppp_integer : ::std::false_type {
};

template <::std::size_t SSize>
struct is_mppp_integer<::mppp::integer<SSize>> : ::std::true_type {
};

template <typename T>
inline constexpr bool is_mppp_integer_v = is_mppp_integer<T>::value;

} // namespace obake::detail

#endif
