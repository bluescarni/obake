// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_SERIES_HPP
#define PIRANHA_SERIES_HPP

#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/math/pow.hpp>

namespace piranha
{

template <typename Cf, typename Key, typename Tag>
class series
{
};

namespace detail
{

template <typename T>
struct is_series_impl : ::std::false_type {
};

template <typename T>
struct is_series_impl<const T> : is_series_impl<T> {
};

template <typename T>
struct is_series_impl<volatile T> : is_series_impl<T> {
};

template <typename T>
struct is_series_impl<const volatile T> : is_series_impl<T> {
};

template <typename Cf, typename Key, typename Tag>
struct is_series_impl<series<Cf, Key, Tag>> : ::std::true_type {
};

} // namespace detail

template <typename T>
using is_series = detail::is_series_impl<T>;

template <typename T>
inline constexpr bool is_series_v = is_series<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Series = is_series_v<T>;

#endif

namespace customisation::internal
{

}

} // namespace piranha

#endif
