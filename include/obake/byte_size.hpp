// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_BYTE_SIZE_HPP
#define OBAKE_BYTE_SIZE_HPP

#include <cstddef>
#include <type_traits>
#include <utility>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#if defined(MPPP_WITH_MPFR)
#include <mp++/real.hpp>
#endif

#include <obake/config.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::byte_size().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto byte_size = not_implemented;

namespace internal
{

// Internal customisation point for obake::byte_size().
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto byte_size = not_implemented;

} // namespace internal

} // namespace customisation

namespace detail
{

// Overloads for the mp++ classes.
// NOTE: for real128, the default implementation
// (based on sizeof()) is correct.
template <::std::size_t SSize>
inline ::std::size_t byte_size(const ::mppp::integer<SSize> &n)
{
    if (n.is_static()) {
        // Static storage, everything
        // is stored within the object.
        return sizeof(n);
    } else {
        // Dynamic storage: add the space
        // occupied by the dynamically-allocated limbs.
        // NOTE: not ideal here to use directly a GMP
        // type, perhaps in the future we can provide
        // a type alias in mp++ or even a function for
        // this computation.
        return sizeof(n) + n.size() * sizeof(::mp_limb_t);
    }
}

template <::std::size_t SSize>
inline ::std::size_t byte_size(const ::mppp::rational<SSize> &q)
{
    // Take care of potential padding.
    static_assert(sizeof(q) >= sizeof(::mppp::integer<SSize>) * 2u);
    const auto pad_size = sizeof(q) - sizeof(::mppp::integer<SSize>) * 2u;

    return detail::byte_size(q.get_num()) + detail::byte_size(q.get_den()) + pad_size;
}

#if defined(MPPP_WITH_MPFR)

inline ::std::size_t byte_size(const ::mppp::real &r)
{
    // Size of r plus the dynamically-allocated storage.
    // NOTE: not ideal here to use directly an MPFR
    // function, perhaps in the future we can provide
    // an implementation in mp++.
    return sizeof(r) + mpfr_custom_get_size(r.get_prec());
}

#endif

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto byte_size_impl(T &&x, priority_tag<3>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::byte_size<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto byte_size_impl(T &&x, priority_tag<2>) OBAKE_SS_FORWARD_FUNCTION(byte_size(::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto byte_size_impl(T &&x, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::internal::byte_size<T &&>)(::std::forward<T>(x)));

// Lowest priority: default implementation, using sizeof().
template <typename T>
constexpr auto byte_size_impl(T &&x, priority_tag<0>) OBAKE_SS_FORWARD_FUNCTION(sizeof(x));

// Machinery to enable the byte_size() implementation only if the return
// type is std::size_t.
template <typename T>
using byte_size_impl_ret_t = decltype(detail::byte_size_impl(::std::declval<T>(), priority_tag<3>{}));

template <typename T, ::std::enable_if_t<::std::is_same_v<detected_t<byte_size_impl_ret_t, T>, ::std::size_t>, int> = 0>
constexpr auto byte_size_impl_with_ret_check(T &&x)
    OBAKE_SS_FORWARD_FUNCTION(detail::byte_size_impl(::std::forward<T>(x), priority_tag<3>{}));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct byte_size_msvc {
    template <typename T>
    constexpr auto operator()(T &&x) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::byte_size_impl_with_ret_check(::std::forward<T>(x)))
};

inline constexpr auto byte_size = byte_size_msvc{};

#else

inline constexpr auto byte_size =
    [](auto &&x) OBAKE_SS_FORWARD_LAMBDA(detail::byte_size_impl_with_ret_check(::std::forward<decltype(x)>(x)));

#endif

namespace detail
{

template <typename T>
using byte_size_t = decltype(::obake::byte_size(::std::declval<T>()));

}

template <typename T>
using is_size_measurable = is_detected<detail::byte_size_t, T>;

template <typename T>
inline constexpr bool is_size_measurable_v = is_size_measurable<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL SizeMeasurable = requires(T &&x)
{
    ::obake::byte_size(::std::forward<T>(x));
};

#endif

} // namespace obake

#endif
