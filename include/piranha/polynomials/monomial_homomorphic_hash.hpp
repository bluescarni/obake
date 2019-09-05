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
#include <piranha/detail/not_implemented.hpp>
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
inline constexpr auto monomial_hash_is_homomorphic = not_implemented;

} // namespace customisation

// Main monomial_hash_is_homomorphic implementation.
// Defaults to false.
template <typename T
#if !defined(PIRANHA_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto monomial_hash_is_homomorphic = false;

namespace detail
{

// Small helper to check if an mhh implementation is constexpr.
template <bool>
void mhh_constexpr_checker();

// Detect the presence of a valid customisation::monomial_hash_is_homomorphic implementation:
// must be a constexpr bool variable.
template <typename T>
using cmhh_checker_t = decltype(customisation::monomial_hash_is_homomorphic<T>);

template <typename T>
using cmhh_constexpr_checker_t
    = decltype(detail::mhh_constexpr_checker<customisation::monomial_hash_is_homomorphic<T>>());

template <typename T>
inline constexpr bool has_cmhh = ::std::conjunction_v<::std::is_same<detected_t<cmhh_checker_t, T>, const bool>,
                                                      is_detected<cmhh_constexpr_checker_t, T>>;

// Do the same for monomial_hash_is_homomorphic.
template <typename T>
using mhh_checker_t = decltype(monomial_hash_is_homomorphic<T>);

template <typename T>
using mhh_constexpr_checker_t = decltype(detail::mhh_constexpr_checker<monomial_hash_is_homomorphic<T>>());

template <typename T>
inline constexpr bool has_mhh = ::std::conjunction_v<::std::is_same<detected_t<mhh_checker_t, T>, const bool>,
                                                     is_detected<mhh_constexpr_checker_t, T>>;

// Implementation of is_homomorphically_hashable_monomial:
// - if we have a valid implementation in the external customisation
//   namespace, use that, otherwise,
// - if we have a valid implementation in the main namespace, use that,
//   otherwise,
// - return false.
template <typename T>
inline constexpr bool is_hhm_impl()
{
    if constexpr (has_cmhh<T>) {
        return customisation::monomial_hash_is_homomorphic<T>;
    } else {
        if constexpr (has_mhh<T>) {
            return monomial_hash_is_homomorphic<T>;
        } else {
            return false;
        }
    }
}

} // namespace detail

// Detect the availability of homomorphic hashing in monomials.
// NOTE: the detection is independent of the availability
// of a hash function.
template <typename T>
using is_homomorphically_hashable_monomial = ::std::integral_constant<bool, detail::is_hhm_impl<T>()>;

template <typename T>
inline constexpr bool is_homomorphically_hashable_monomial_v = is_homomorphically_hashable_monomial<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL HomomorphicallyHashableMonomial = is_homomorphically_hashable_monomial_v<T>;

#endif

} // namespace piranha

#endif
