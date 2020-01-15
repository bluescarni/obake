// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POLYNOMIALS_MONOMIAL_HOMOMORPHIC_HASH_HPP
#define OBAKE_POLYNOMIALS_MONOMIAL_HOMOMORPHIC_HASH_HPP

#include <cstddef>
#include <type_traits>

#include <obake/config.hpp>
#include <obake/detail/not_implemented.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

// NOTE: for homomorphic hashing to be usable, we need
// to make sure that std::size_t is not subject to any integral
// promotion. An integral promotion could in theory
// convert std::size_t to a signed type when we do an addition
// of hashes, which could then result in UB in case of overflows
// (instead of modulo arithmetics).
static_assert(::std::is_same_v<::std::common_type_t<::std::size_t>, ::std::size_t>,
              "std::size_t cannot be subject to integral promotions.");

namespace customisation
{

// External customisation point for monomial_hash_is_homomorphic.
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto monomial_hash_is_homomorphic = not_implemented;

} // namespace customisation

// Main monomial_hash_is_homomorphic implementation.
// Defaults to false.
template <typename T
#if !defined(OBAKE_HAVE_CONCEPTS)
          ,
          typename = void
#endif
          >
inline constexpr auto monomial_hash_is_homomorphic = false;

namespace detail
{

// Detect the presence of a valid customisation::monomial_hash_is_homomorphic implementation:
// must be a const bool variable.
template <typename T>
using c_monomial_hh_checker_t = decltype(customisation::monomial_hash_is_homomorphic<T>);

template <typename T>
inline constexpr bool has_c_monomial_hh = ::std::is_same_v<detected_t<c_monomial_hh_checker_t, T>, const bool>;

// Do the same for monomial_hash_is_homomorphic.
template <typename T>
using monomial_hh_checker_t = decltype(monomial_hash_is_homomorphic<T>);

template <typename T>
inline constexpr bool has_monomial_hh = ::std::is_same_v<detected_t<monomial_hh_checker_t, T>, const bool>;

// Implementation of is_homomorphically_hashable_monomial:
// - if we have a valid implementation in the external customisation
//   namespace, use that, otherwise,
// - if we have a valid implementation in the main namespace, use that,
//   otherwise,
// - return false.
template <typename T>
constexpr bool is_hh_monomial_impl()
{
    if constexpr (has_c_monomial_hh<T>) {
        return customisation::monomial_hash_is_homomorphic<T>;
    } else {
        if constexpr (has_monomial_hh<T>) {
            return monomial_hash_is_homomorphic<T>;
        } else {
            // NOTE: we end up here only if the user screwed up
            // the customisation in the main namespace. In such case,
            // just return false for safety.
            return false;
        }
    }
}

} // namespace detail

// Detect the availability of homomorphic hashing in monomials.
// NOTE: the detection is independent of the availability
// of a hash function.
// NOTE: the implementation ensures that the type trait is always
// usable (i.e., using it will never generate any sort of error,
// if errors in the user implementations arise, we get the default
// behaviour).
template <typename T>
using is_homomorphically_hashable_monomial = ::std::integral_constant<bool, detail::is_hh_monomial_impl<T>()>;

template <typename T>
inline constexpr bool is_homomorphically_hashable_monomial_v = is_homomorphically_hashable_monomial<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL HomomorphicallyHashableMonomial = is_homomorphically_hashable_monomial_v<T>;

#endif

} // namespace obake

#endif
