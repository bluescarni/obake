// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_TUPLE_FOR_EACH_HPP
#define OBAKE_DETAIL_TUPLE_FOR_EACH_HPP

#include <cstddef>
#include <tuple>
#include <utility>

#include <obake/type_traits.hpp>

namespace obake::detail
{

// NOTE: MSVC2015 and earlier don't have std::apply().
#if defined(_MSC_VER) && _MSC_VER < 1910

template <typename T, typename F, ::std::size_t... Is>
inline void apply_to_each_item(T &&t, F &&f, ::std::index_sequence<Is...>)
{
    (void(::std::forward<F>(f)(::std::get<Is>(::std::forward<T>(t)))), ...);
}

template <typename Tuple, typename F>
inline void tuple_for_each(Tuple &&t, F &&f)
{
    detail::apply_to_each_item(::std::forward<Tuple>(t), ::std::forward<F>(f),
                               ::std::make_index_sequence<::std::tuple_size_v<remove_cvref_t<Tuple>>>{});
}

#else

// Tuple for_each(). It will apply the input functor f to each element of
// the input tuple tup, sequentially.
template <typename Tuple, typename F>
inline void tuple_for_each(Tuple &&tup, F &&f)
{
    ::std::apply(
        [&f](auto &&... items) {
            // NOTE: here we are converting to void the results of the invocations
            // of f. This ensures that we are folding using the builtin comma
            // operator, which implies sequencing:
            // """
            //  Every value computation and side effect of the first (left) argument of the built-in comma operator is
            //  sequenced before every value computation and side effect of the second (right) argument.
            // """
            // NOTE: we are writing this as a right fold, i.e., it will expand as:
            //
            // f(tup[0]), (f(tup[1]), (f(tup[2])...
            //
            // A left fold would also work guaranteeing the same sequencing.
            (void(::std::forward<F>(f)(::std::forward<decltype(items)>(items))), ...);
        },
        ::std::forward<Tuple>(tup));
}

#endif

} // namespace obake::detail

#endif
