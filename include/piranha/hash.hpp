// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_HASH_HPP
#define PIRANHA_HASH_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/not_implemented.hpp>
#include <piranha/detail/priority_tag.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace customisation
{

// External customisation point for piranha::hash().
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto hash = not_implemented;

} // namespace customisation

namespace detail
{

// Highest priority: explicit user override in the external customisation namespace.
template <typename T>
constexpr auto hash_impl(T &&x, priority_tag<2>)
    PIRANHA_SS_FORWARD_FUNCTION((customisation::hash<T &&>)(::std::forward<T>(x)));

// Unqualified function call implementation.
template <typename T>
constexpr auto hash_impl(T &&x, priority_tag<1>) PIRANHA_SS_FORWARD_FUNCTION(hash(::std::forward<T>(x)));

// Lowest priority: try to use std::hash.
// NOTE: MSVC 2015 and earlier don't have a poisoned std::hash:
// if it is instantiated with an unsupported type, it will error
// out with a static assert instead of sfinaeing. As a compromise,
// enable this implementation only for a few known types.
#if defined(_MSC_VER) && _MSC_VER < 1910

template <typename T, ::std::enable_if_t<::std::disjunction_v<::std::is_arithmetic<remove_cvref_t<T>>,
                                                              ::std::is_same<::std::string, remove_cvref_t<T>>>,
                                         int> = 0>
constexpr ::std::size_t hash_impl(T &&x, priority_tag<0>)
{
    return ::std::hash<remove_cvref_t<T>>{}(::std::forward<T>(x));
}

#else

template <typename T>
constexpr auto hash_impl(T &&x, priority_tag<0>)
    PIRANHA_SS_FORWARD_FUNCTION(::std::hash<remove_cvref_t<T>>{}(::std::forward<T>(x)));

#endif

// Machinery to enable the hash implementation only if the return
// type is std::size_t.
template <typename T>
using hash_impl_ret_t = decltype(detail::hash_impl(::std::declval<T>(), priority_tag<2>{}));

template <typename T, ::std::enable_if_t<::std::is_same_v<detected_t<hash_impl_ret_t, T>, ::std::size_t>, int> = 0>
constexpr auto hash_impl_with_ret_check(T &&x)
    PIRANHA_SS_FORWARD_FUNCTION(detail::hash_impl(::std::forward<T>(x), priority_tag<2>{}));

} // namespace detail

#if defined(_MSC_VER)

struct hash_msvc {
    template <typename T>
    constexpr auto operator()(T &&x) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(detail::hash_impl_with_ret_check(::std::forward<T>(x)))
};

inline constexpr auto hash = hash_msvc{};

#else

inline constexpr auto hash =
    [](auto &&x) PIRANHA_SS_FORWARD_LAMBDA(detail::hash_impl_with_ret_check(::std::forward<decltype(x)>(x)));

#endif

namespace detail
{

template <typename T>
using hash_t = decltype(::piranha::hash(::std::declval<T>()));

}

template <typename T>
using is_hashable = is_detected<detail::hash_t, T>;

template <typename T>
inline constexpr bool is_hashable_v = is_hashable<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Hashable = requires(T &&x)
{
    ::piranha::hash(::std::forward<T>(x));
};

#endif

namespace customisation
{

// External customisation point for hash_is_homomorphic.
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
struct hash_is_homomorphic {
};

} // namespace customisation

// Main hash_is_homomorphic implementation.
// Defaults to false.
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
struct hash_is_homomorphic : ::std::false_type {
};

namespace detail
{

// Detect the presence of a valid customisation::hash_is_homomorphic implementation
// (i.e., must have static const bool value member).
template <typename T>
using customised_hash_is_homomorphic_t = decltype(customisation::hash_is_homomorphic<T>::value);

template <typename T>
using has_customised_hash_is_homomorphic = ::std::is_same<detected_t<customised_hash_is_homomorphic_t, T>, const bool>;

// Detect the presence of a valid hash_is_homomorphic implementation
// (i.e., must have static const bool value member).
template <typename T>
using hash_is_homomorphic_t = decltype(hash_is_homomorphic<T>::value);

template <typename T>
using has_hash_is_homomorphic = ::std::is_same<detected_t<hash_is_homomorphic_t, T>, const bool>;

// Implementation of has_homomorphic_hash:
// - if a valid implementation of customisation::hash_is_homomorphic is provided,
//   use that, otherwise,
// - if a valid implementation of hash_is_homomorphic is provided,
//   use that, otherwise,
// - defaults to false.
template <typename T, typename = void>
struct has_homomorphic_hash_impl
    : ::std::conditional_t<has_hash_is_homomorphic<T>::value, hash_is_homomorphic<T>, ::std::false_type> {
};

template <typename T>
struct has_homomorphic_hash_impl<T, ::std::enable_if_t<has_customised_hash_is_homomorphic<T>::value>>
    : customisation::hash_is_homomorphic<T> {
};

} // namespace detail

// Detect the availability of homomorphic hashing.
// NOTE: the detection is independent of the availability
// of a hash function.
// NOTE: remove cvref for ease of use.
template <typename T>
using has_homomorphic_hash = detail::has_homomorphic_hash_impl<remove_cvref_t<T>>;

template <typename T>
inline constexpr bool has_homomorphic_hash_v = has_homomorphic_hash<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL HasHomomorphicHash = has_homomorphic_hash_v<T>;

#endif

} // namespace piranha

#endif
