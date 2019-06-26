// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP
#define PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP

#include <type_traits>
#include <utility>
#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/type_c.hpp>
#include <piranha/polynomials/monomial_range_overflow_check.hpp>
#include <piranha/ranges.hpp>
#include <piranha/series.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace polynomials
{

struct tag {
};

template <typename K, typename C>
using polynomial = series<K, C, tag>;

namespace detail
{

template <typename T>
struct is_polynomial_impl : ::std::false_type {
};

template <typename K, typename C>
struct is_polynomial_impl<polynomial<K, C>> : ::std::true_type {
};

template <typename T>
using is_polynomial = is_polynomial_impl<T>;

template <typename T>
inline constexpr bool is_polynomial_v = is_polynomial<T>::value;

template <typename T, typename U>
constexpr auto poly_mul_algorithm_impl()
{
    // Shortcut for signalling that the mul implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, ::piranha::detail::type_c<void>{});

    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    if constexpr (::std::disjunction_v<::std::negation<is_polynomial<rT>>, ::std::negation<is_polynomial<rU>>>) {
        // T and U are not both polynomials.
        return failure;
    } else {
        constexpr auto rank_T = series_rank<rT>;
        constexpr auto rank_U = series_rank<rU>;

        if constexpr (rank_T != rank_U) {
            // T and U are both polynomials, but with different rank.
            return failure;
        } else if constexpr (!::std::is_same_v<series_key_t<rT>, series_key_t<rU>>) {
            // The key types differ.
            return failure;
        } else {
            // T and U are both polynomials, same rank, same key.
            // Determine if we can multiply the coefficients, using
            // const lvalue refs.
            using ret_cf_t = detected_t<::piranha::detail::mul_t, const series_cf_t<rT> &, const series_cf_t<rU> &>;

            if constexpr (is_cf_v<ret_cf_t>) {
                return ::std::make_pair(1, ::piranha::detail::type_c<polynomial<series_key_t<rT>, ret_cf_t>>{});
            } else {
                return failure;
            }
        }
    }
}

// Shortcuts.
template <typename T, typename U>
inline constexpr auto poly_mul_algorithm = detail::poly_mul_algorithm_impl<T, U>();

template <typename T, typename U>
inline constexpr int poly_mul_algo = poly_mul_algorithm<T, U>.first;

template <typename T, typename U>
using poly_mul_ret_t = typename decltype(poly_mul_algorithm<T, U>.second)::type;

struct poly_term_key_ref_extractor {
    template <typename P>
    decltype(auto) operator()(const P &p) const
    {
        return static_cast<const typename P::first_type &>(p.first);
    }
};

// Implementation of poly multiplication with identical symbol sets.
template <typename T, typename U>
inline auto poly_mul_impl(T &&x, U &&y)
{
    using ret_t = poly_mul_ret_t<T &&, U &&>;
    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;

    assert(x.get_symbol_set() == y.get_symbol_set());
    const auto &ss = x.get_symbol_set();

    // Create vectors containing copies of
    // the input terms.
    // TODO: move coefficients if possible? use rref cleaner.
    auto p_transform = [](const auto &p) { return ::std::make_pair(p.first, p.second); };
    ::std::vector<::std::pair<series_key_t<rT>, series_cf_t<rT>>> v1(
        ::boost::make_transform_iterator(x.cbegin(), p_transform),
        ::boost::make_transform_iterator(x.cend(), p_transform));
    ::std::vector<::std::pair<series_key_t<rU>, series_cf_t<rU>>> v2(
        ::boost::make_transform_iterator(y.cbegin(), p_transform),
        ::boost::make_transform_iterator(y.cend(), p_transform));

    // Do the monomial overflow checking.
    // TODO: add monomial_range_overflow_check() requirement
    // TODO: testing of make_range().
    // in the algorithm determination.
    const bool overflow = !::piranha::monomial_range_overflow_check(
        ::piranha::detail::make_range(::boost::make_transform_iterator(v1.cbegin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(v1.cend(), poly_term_key_ref_extractor{})),
        ::piranha::detail::make_range(::boost::make_transform_iterator(v2.cbegin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(v2.cend(), poly_term_key_ref_extractor{})),
        ss);

    if (piranha_unlikely(overflow)) {
        // TODO error message.
        piranha_throw(::std::overflow_error, "");
    }

    return ret_t{};
}

} // namespace detail

template <typename T, typename U, ::std::enable_if_t<detail::poly_mul_algo<T &&, U &&> != 0, int> = 0>
constexpr detail::poly_mul_ret_t<T &&, U &&> series_mul(T &&x, U &&y)
{
    if (x.get_symbol_set() == y.get_symbol_set()) {
        return detail::poly_mul_impl(::std::forward<T>(x), ::std::forward<U>(y));
    } else {
        return detail::poly_mul_ret_t<T &&, U &&>{};
    }
}

} // namespace polynomials

// Lift to the piranha namespace.
template <typename C, typename K>
using polynomial = polynomials::polynomial<C, K>;

} // namespace piranha

#endif
