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
template <typename T>
constexpr decltype(auto) tcast(T &&x) noexcept
{
    if constexpr (::std::conjunction_v<::std::is_rvalue_reference<T &&>,
                                       ::std::negation<::std::is_const<::std::remove_reference_t<T &&>>>>) {
        return static_cast<T &&>(x);
    } else {
        return static_cast<const remove_cvref_t<T> &>(x);
    }
}

} // namespace piranha::detail

#endif
