// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_SAFE_CONVERT_HPP
#define OBAKE_MATH_SAFE_CONVERT_HPP

#include <cstddef>
#include <type_traits>
#include <utility>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/detail/priority_tag.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace customisation
{

// External customisation point for obake::safe_convert().
template <typename T, typename U
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto safe_convert = not_implemented;

} // namespace customisation

namespace detail
{

// Implementation for C++ integrals.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
    requires Integral<T> && !::std::is_const_v<T> && Integral<U>
#else
template <typename T, typename U,
          ::std::enable_if_t<::std::conjunction_v<is_integral<T>, ::std::negation<::std::is_const<T>>, is_integral<U>>,
                             int> = 0>
#endif
    constexpr bool safe_convert(T &out, const U &n_orig) noexcept
{
    // Small helpers to get the min/max values of type T.
    // For bool, they will cast the return type to unsigned in
    // order to avoid compiler warnings.
    [[maybe_unused]] constexpr auto Tmin = []() {
        if constexpr (::std::is_same_v<T, bool>) {
            return 0u;
        } else {
            return limits_min<T>;
        }
    }();
    constexpr auto Tmax = []() {
        if constexpr (::std::is_same_v<T, bool>) {
            return 1u;
        } else {
            return limits_max<T>;
        }
    }();

    // Promote n to unsigned, if it is of type bool, in order
    // to avoid compiler warnings.
    const auto n = [&n_orig]() {
        if constexpr (::std::is_same_v<U, bool>) {
            return static_cast<unsigned>(n_orig);
        } else {
            return n_orig;
        }
    }();

    if constexpr (is_signed_v<T> == is_signed_v<U>) {
        // Same signedness, we can use direct comparisons.
        if (n >= Tmin && n <= Tmax) {
            out = static_cast<T>(n);
            return true;
        }
        return false;
    } else {
        if constexpr (is_signed_v<T>) {
            // T signed, U unsigned.
            if (n <= static_cast<make_unsigned_t<T>>(Tmax)) {
                out = static_cast<T>(n);
                return true;
            }
            return false;
        } else {
            // T unsigned, U signed.
            if (n < U(0) || static_cast<make_unsigned_t<U>>(n) > Tmax) {
                return false;
            }
            out = static_cast<T>(n);
            return true;
        }
    }
}

// Implementations for mppp::integer - C++ integrals.
#if defined(OBAKE_HAVE_CONCEPTS)
template <::std::size_t SSize, Integral T>
#else
template <::std::size_t SSize, typename T, ::std::enable_if_t<is_integral_v<T>, int> = 0>
#endif
inline bool safe_convert(::mppp::integer<SSize> &n, const T &m)
{
    n = m;
    return true;
}

#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, ::std::size_t SSize>
requires Integral<T> && !::std::is_const_v<T>
#else
template <typename T, ::std::size_t SSize,
          ::std::enable_if_t<::std::conjunction_v<is_integral<T>, ::std::negation<::std::is_const<T>>>, int> = 0>
#endif
inline bool safe_convert(T &n, const ::mppp::integer<SSize> &m)
{
    return ::mppp::get(n, m);
}

// Implementations for mppp::integer - mppp::rational.
// NOTE: here potentially we could take advantage
// of move operations, if the second argument is an rvalue
// reference.
template <::std::size_t SSize>
inline bool safe_convert(::mppp::integer<SSize> &n, const ::mppp::rational<SSize> &q)
{
    if (q.get_den().is_one()) {
        n = q.get_num();
        return true;
    } else {
        return false;
    }
}

template <::std::size_t SSize>
inline bool safe_convert(::mppp::rational<SSize> &q, const ::mppp::integer<SSize> &n)
{
    q = n;
    return true;
}

// Implementations for C++ integrals - mppp::rational.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, ::std::size_t SSize>
    requires Integral<T> && !::std::is_const_v<T>
#else
template <typename T, ::std::size_t SSize,
          ::std::enable_if_t<::std::conjunction_v<is_integral<T>, ::std::negation<::std::is_const<T>>>, int> = 0>
#endif
inline bool safe_convert(T &n, const ::mppp::rational<SSize> &q)
{
    if (q.get_den().is_one()) {
        return ::mppp::get(n, q.get_num());
    } else {
        return false;
    }
}

#if defined(OBAKE_HAVE_CONCEPTS)
template <Integral T, ::std::size_t SSize>
#else
template <typename T, ::std::size_t SSize, ::std::enable_if_t<is_integral_v<T>, int> = 0>
#endif
inline bool safe_convert(::mppp::rational<SSize> &q, const T &n)
{
    q = n;
    return true;
}

// Highest priority: explicit user override in the external customisation namespace.
template <typename T, typename U>
constexpr auto safe_convert_impl(T &&x, U &&y, priority_tag<2>)
    OBAKE_SS_FORWARD_FUNCTION((customisation::safe_convert<T &&, U &&>)(::std::forward<T>(x), ::std::forward<U>(y)));

// Unqualified function call implementation.
template <typename T, typename U>
constexpr auto safe_convert_impl(T &&x, U &&y, priority_tag<1>)
    OBAKE_SS_FORWARD_FUNCTION(safe_convert(::std::forward<T>(x), ::std::forward<U>(y)));

// Lowest priority: it will assign y to x, but only if T and U are the same type,
// after the removal of reference and cv qualifiers.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename T, typename U>
requires ::std::is_same_v<remove_cvref_t<T>, remove_cvref_t<U>>
#else
template <typename T, typename U, ::std::enable_if_t<::std::is_same_v<remove_cvref_t<T>, remove_cvref_t<U>>, int> = 0>
#endif
    constexpr auto safe_convert_impl(T &&x, U &&y, priority_tag<0>)
        OBAKE_SS_FORWARD_FUNCTION((void(::std::forward<T>(x) = ::std::forward<U>(y)), true));

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

struct safe_convert_msvc {
    template <typename T, typename U>
    constexpr auto operator()(T &&x, U &&y) const OBAKE_SS_FORWARD_MEMBER_FUNCTION(static_cast<bool>(
        detail::safe_convert_impl(::std::forward<T>(x), ::std::forward<U>(y), detail::priority_tag<2>{})))
};

inline constexpr auto safe_convert = safe_convert_msvc{};

#else

inline constexpr auto safe_convert =
    [](auto &&x, auto &&y) OBAKE_SS_FORWARD_LAMBDA(static_cast<bool>(detail::safe_convert_impl(
        ::std::forward<decltype(x)>(x), ::std::forward<decltype(y)>(y), detail::priority_tag<2>{})));

#endif

namespace detail
{

template <typename T, typename U>
using safe_convert_t = decltype(::obake::safe_convert(::std::declval<T>(), ::std::declval<U>()));

}

template <typename From, typename To>
using is_safely_convertible = is_detected<detail::safe_convert_t, To, From>;

template <typename From, typename To>
inline constexpr bool is_safely_convertible_v = is_safely_convertible<From, To>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename From, typename To>
OBAKE_CONCEPT_DECL SafelyConvertible = requires(From &&x, To &&y)
{
    ::obake::safe_convert(::std::forward<To>(y), ::std::forward<From>(x));
};

#endif

} // namespace obake

#endif
