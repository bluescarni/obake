// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_MONOMIAL_HOMOMORPHIC_HASH_HPP
#define PIRANHA_POLYNOMIALS_MONOMIAL_HOMOMORPHIC_HASH_HPP

#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace customisation
{

// External customisation point for monomial_hash_is_homomorphic.
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
struct monomial_hash_is_homomorphic {
};

} // namespace customisation

// Main monomial_hash_is_homomorphic implementation.
// Defaults to false.
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
struct monomial_hash_is_homomorphic : ::std::false_type {
};

namespace detail
{

// Detect the presence of a valid customisation::monomial_hash_is_homomorphic implementation
// (i.e., must have static const bool value member).
template <typename T>
using customised_monomial_hash_is_homomorphic_t = decltype(customisation::monomial_hash_is_homomorphic<T>::value);

template <typename T>
using has_customised_monomial_hash_is_homomorphic
    = ::std::is_same<detected_t<customised_monomial_hash_is_homomorphic_t, T>, const bool>;

// Detect the presence of a valid monomial_hash_is_homomorphic implementation
// (i.e., must have static const bool value member).
template <typename T>
using monomial_hash_is_homomorphic_t = decltype(monomial_hash_is_homomorphic<T>::value);

template <typename T>
using has_monomial_hash_is_homomorphic = ::std::is_same<detected_t<monomial_hash_is_homomorphic_t, T>, const bool>;

// Implementation of is_homomorphically_hashable_monomial:
// - if a valid implementation of customisation::monomial_hash_is_homomorphic is provided,
//   use that, otherwise,
// - if a valid implementation of monomial_hash_is_homomorphic is provided,
//   use that, otherwise,
// - defaults to false.
template <typename T, typename = void>
struct is_homomorphically_hashable_monomial_impl
    : ::std::conditional_t<has_monomial_hash_is_homomorphic<T>::value, monomial_hash_is_homomorphic<T>,
                           ::std::false_type> {
};

template <typename T>
struct is_homomorphically_hashable_monomial_impl<
    T, ::std::enable_if_t<has_customised_monomial_hash_is_homomorphic<T>::value>>
    : customisation::monomial_hash_is_homomorphic<T> {
};

} // namespace detail

// Detect the availability of homomorphic hashing in monomials.
// NOTE: the detection is independent of the availability
// of a hash function.
template <typename T>
using is_homomorphically_hashable_monomial = detail::is_homomorphically_hashable_monomial_impl<T>;

template <typename T>
inline constexpr bool is_homomorphically_hashable_monomial_v = is_homomorphically_hashable_monomial<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL HomomorphicallyHashableMonomial = is_homomorphically_hashable_monomial_v<T>;

#endif

} // namespace piranha

#endif
