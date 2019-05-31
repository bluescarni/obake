// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_TCAST_HPP
#define PIRANHA_DETAIL_TCAST_HPP

#include <type_traits>

#include <piranha/type_traits.hpp>

namespace piranha::detail
{

// A transforming cast: if the input is a nonconst
// rvalue reference, return it, otherwise transform
// it into a const lvalue reference.

#if defined(_MSC_VER) && !defined(__clang__)

template <typename T>
constexpr T &&tcast_impl(T &&x, ::std::true_type) noexcept
{
    return static_cast<T &&>(x);
}

template <typename T>
constexpr const remove_cvref_t<T> &tcast_impl(T &&x, ::std::false_type) noexcept
{
    return static_cast<const remove_cvref_t<T> &>(x);
}

template <typename T>
constexpr decltype(auto) tcast(T &&x) noexcept
{
    return detail::tcast_impl(::std::forward<T>(x), is_mutable_rvalue_reference<T &&>{});
}

#else

template <typename T>
constexpr decltype(auto) tcast(T &&x) noexcept
{
    if constexpr (is_mutable_rvalue_reference_v<T &&>) {
        return static_cast<T &&>(x);
    } else {
        return static_cast<const remove_cvref_t<T> &>(x);
    }
}

#endif

} // namespace piranha::detail

#endif
