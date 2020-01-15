// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_RANGES_HPP
#define OBAKE_RANGES_HPP

#include <iterator>
#include <type_traits>
#include <utility>

#include <obake/config.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace detail
{

namespace begin_using_adl
{

using ::std::begin;

template <typename T>
constexpr auto call_begin(T &&x) OBAKE_SS_FORWARD_FUNCTION(begin(::std::forward<T>(x)));

template <typename T>
using begin_t = decltype(begin_using_adl::call_begin(::std::declval<T>()));

} // namespace begin_using_adl

namespace end_using_adl
{

using ::std::end;

template <typename T>
constexpr auto call_end(T &&x) OBAKE_SS_FORWARD_FUNCTION(end(::std::forward<T>(x)));

template <typename T>
using end_t = decltype(end_using_adl::call_end(::std::declval<T>()));

} // namespace end_using_adl

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T>
requires Iterator<begin_using_adl::begin_t<T>>
#else
template <typename T, ::std::enable_if_t<is_iterator_v<detected_t<begin_using_adl::begin_t, T>>, int> = 0>
#endif
    constexpr auto begin_impl(T &&x) OBAKE_SS_FORWARD_FUNCTION(begin_using_adl::call_begin(::std::forward<T>(x)));

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T>
requires Iterator<end_using_adl::end_t<T>>
#else
template <typename T, ::std::enable_if_t<is_iterator_v<detected_t<end_using_adl::end_t, T>>, int> = 0>
#endif
    constexpr auto end_impl(T &&x) OBAKE_SS_FORWARD_FUNCTION(end_using_adl::call_end(::std::forward<T>(x)));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct begin_msvc {
    template <typename T>
    constexpr auto operator()(T &&x) const OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::begin_impl(::std::forward<T>(x)))
};

inline constexpr auto begin = begin_msvc{};

struct end_msvc {
    template <typename T>
    constexpr auto operator()(T &&x) const OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::end_impl(::std::forward<T>(x)))
};

inline constexpr auto end = end_msvc{};

#else

inline constexpr auto begin = [](auto &&x) OBAKE_SS_FORWARD_LAMBDA(detail::begin_impl(::std::forward<decltype(x)>(x)));

inline constexpr auto end = [](auto &&x) OBAKE_SS_FORWARD_LAMBDA(detail::end_impl(::std::forward<decltype(x)>(x)));

#endif

template <typename T>
using range_begin_t = decltype(::obake::begin(::std::declval<T>()));

template <typename T>
using range_end_t = decltype(::obake::end(::std::declval<T>()));

template <typename T>
using is_range = ::std::conjunction<is_detected<range_begin_t, T>, is_detected<range_end_t, T>,
                                    ::std::is_same<detected_t<range_begin_t, T>, detected_t<range_end_t, T>>>;

template <typename T>
inline constexpr bool is_range_v = is_range<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL Range = ::std::is_same_v<range_begin_t<T>, range_end_t<T>>;

#endif

template <typename T>
using is_input_range = ::std::conjunction<is_range<T>, is_input_iterator<detected_t<range_begin_t, T>>>;

template <typename T>
inline constexpr bool is_input_range_v = is_input_range<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL InputRange = Range<T> &&InputIterator<range_begin_t<T>>;

#endif

template <typename T>
using is_forward_range = ::std::conjunction<is_range<T>, is_forward_iterator<detected_t<range_begin_t, T>>>;

template <typename T>
inline constexpr bool is_forward_range_v = is_forward_range<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL ForwardRange = Range<T> &&ForwardIterator<range_begin_t<T>>;

#endif

template <typename T>
using is_mutable_forward_range
    = ::std::conjunction<is_range<T>, is_mutable_forward_iterator<detected_t<range_begin_t, T>>>;

template <typename T>
inline constexpr bool is_mutable_forward_range_v = is_mutable_forward_range<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL MutableForwardRange = Range<T> &&MutableForwardIterator<range_begin_t<T>>;

#endif

template <typename T>
using is_bidirectional_range = ::std::conjunction<is_range<T>, is_bidirectional_iterator<detected_t<range_begin_t, T>>>;

template <typename T>
inline constexpr bool is_bidirectional_range_v = is_bidirectional_range<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL BidirectionalRange = Range<T> &&BidirectionalIterator<range_begin_t<T>>;

#endif

template <typename T>
using is_random_access_range = ::std::conjunction<is_range<T>, is_random_access_iterator<detected_t<range_begin_t, T>>>;

template <typename T>
inline constexpr bool is_random_access_range_v = is_random_access_range<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL RandomAccessRange = Range<T> &&RandomAccessIterator<range_begin_t<T>>;

#endif

namespace detail
{

// Machinery to construct a minimal range
// type from a pair of begin/end iterators.
// This is useful when we have functions taking
// ranges in input and we want to use them
// with iterator pairs instead.
namespace range_impl
{

template <typename T>
struct range {
    T b;
    T e;
};

template <typename T>
constexpr T begin(const range<T> &r) noexcept(::std::is_nothrow_copy_constructible_v<T>)
{
    return r.b;
}

template <typename T>
constexpr T end(const range<T> &r) noexcept(::std::is_nothrow_copy_constructible_v<T>)
{
    return r.e;
}

} // namespace range_impl

template <typename T>
constexpr auto make_range(T b, T e) noexcept(::std::is_nothrow_copy_constructible_v<T>)
{
    return range_impl::range<T>{b, e};
}

} // namespace detail

} // namespace obake

#endif
