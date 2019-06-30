// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_COST_MODEL_HPP
#define PIRANHA_COST_MODEL_HPP

#include <cstddef>
#include <type_traits>
#include <utility>

#include <mp++/config.hpp>
#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#if defined(MPPP_WITH_MPFR)

#include <mp++/real.hpp>

#endif

#if defined(MPPP_WITH_QUADMATH)

#include <mp++/real128.hpp>

#endif

#include <piranha/config.hpp>
#include <piranha/detail/not_implemented.hpp>
#include <piranha/detail/priority_tag.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace customisation
{

// External customisation point for piranha::cost_model().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto cost_model = not_implemented;

namespace internal
{

// Internal customisation point for piranha::cost_model().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto cost_model = not_implemented;

} // namespace internal

} // namespace customisation

namespace detail
{

// Specialisation for C++ arithmetic types, including
// 128bit ints if available.
#if defined(PIRANHA_HAVE_CONCEPTS)
template <Arithmetic T>
#else
template <typename T, ::std::enable_if_t<is_arithmetic_v<T>, int> = 0>
#endif
constexpr double cost_model(const T &)
{
    return 1;
}

// NOTE: return fixed costs for the mppp classes for now,
// under the assumption we will be using them with not too many
// bits of precision. In the future we might provide more
// accurate implementations.
template <::std::size_t SSize>
inline double cost_model(const ::mppp::integer<SSize> &)
{
    return 10;
}

template <::std::size_t SSize>
inline double cost_model(const ::mppp::rational<SSize> &)
{
    return 50;
}

#if defined(MPPP_WITH_MPFR)

inline double cost_model(const ::mppp::real &)
{
    return 100;
}

#endif

#if defined(MPPP_WITH_QUADMATH)

inline double cost_model(const ::mppp::real128 &)
{
    return 10;
}

#endif

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto cost_model_impl(T &&x, priority_tag<2>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::cost_model<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto cost_model_impl(T &&x, priority_tag<1>) PIRANHA_SS_FORWARD_FUNCTION(cost_model(::std::forward<T>(x)));

// Explicit override in the internal customisation namespace.
template <typename T>
constexpr auto cost_model_impl(T &&x, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::internal::cost_model<T &&>)(::std::forward<T>(x)));

} // namespace detail

#if defined(_MSC_VER)

struct cost_model_msvc {
    template <typename T>
    constexpr auto operator()(T &&x) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(static_cast<double>(detail::cost_model_impl(::std::forward<T>(x),
                                                                                       detail::priority_tag<2>{})))
};

inline constexpr auto cost_model = cost_model_msvc{};

#else

// NOTE: forcibly cast to double precision.
inline constexpr auto cost_model = [](auto &&x) PIRANHA_SS_FORWARD_LAMBDA(
    static_cast<double>(detail::cost_model_impl(::std::forward<decltype(x)>(x), detail::priority_tag<2>{})));

#endif

namespace detail
{

template <typename T>
using cost_model_t = decltype(::piranha::cost_model(::std::declval<T>()));

}

template <typename T>
using has_cost_model = is_detected<detail::cost_model_t, T>;

template <typename T>
inline constexpr bool has_cost_model_v = has_cost_model<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL CostModelable = requires(T &&x)
{
    ::piranha::cost_model(::std::forward<T>(x));
};

#endif

} // namespace piranha

#endif
