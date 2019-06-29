// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP
#define PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP

#include <array>
#include <cassert>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include <piranha/config.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/detail/type_c.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/math/fma3.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/polynomials/monomial_mul.hpp>
#include <piranha/polynomials/monomial_range_overflow_check.hpp>
#include <piranha/ranges.hpp>
#include <piranha/series.hpp>
#include <piranha/symbols.hpp>
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

// Small helper to extract a const reference to
// a term's key (that is, the first element of the
// input pair p).
struct poly_term_key_ref_extractor {
    template <typename P>
    constexpr decltype(auto) operator()(const P &p) const noexcept
    {
        return static_cast<const typename P::first_type &>(p.first);
    }
};

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
            using cf1_t = series_cf_t<rT>;
            using cf2_t = series_cf_t<rU>;
            using ret_cf_t = detected_t<::piranha::detail::mul_t, const cf1_t &, const cf2_t &>;

            if constexpr (is_cf_v<ret_cf_t>) {
                // The coefficient is valid.
                //
                // We need to perform monomial range checking on the input series.
                // Depending on runtime conditions, the check will be done on either
                // vector of copies of the original terms, or directly on the terms of the
                // series operands.

                // The vectors of copies of input terms.
                using xv_t = ::std::vector<::std::pair<series_key_t<rT>, cf1_t>>;
                using yv_t = ::std::vector<::std::pair<series_key_t<rU>, cf2_t>>;

                // The corresponding range types.
                using xr1_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<xv_t &>().cbegin(), poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<xv_t &>().cend(), poly_term_key_ref_extractor{})));
                using yr1_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<yv_t &>().cbegin(), poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<yv_t &>().cend(), poly_term_key_ref_extractor{})));

                // The original range types. We will be using const_iterator rvalues from the input series.
                using xr2_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<typename rT::const_iterator>(),
                                                     poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<typename rT::const_iterator>(),
                                                     poly_term_key_ref_extractor{})));
                using yr2_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<typename rU::const_iterator>(),
                                                     poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<typename rU::const_iterator>(),
                                                     poly_term_key_ref_extractor{})));

                if constexpr (::std::conjunction_v<
                                  is_overflow_testable_monomial_range<xr1_t, yr1_t>,
                                  is_overflow_testable_monomial_range<xr2_t, yr2_t>,
                                  // We may need to merge new symbols into the original key type.
                                  // NOTE: the key types of T and U must be identical at the moment,
                                  // so checking only T's key type is enough.
                                  // NOTE: the merging is done via a const ref.
                                  is_symbols_mergeable_key<const series_key_t<rT> &>,
                                  // Need to be able to multiply monomials. We work with lvalue
                                  // references, const for the arguments, mutable for the
                                  // return value.
                                  // NOTE: the key types of T and U must be identical at the moment,
                                  // so checking only T's key type is enough.
                                  is_multipliable_monomial<series_key_t<rT> &, const series_key_t<rT> &,
                                                           const series_key_t<rT> &>>) {
                    return ::std::make_pair(1, ::piranha::detail::type_c<polynomial<series_key_t<rT>, ret_cf_t>>{});
                } else {
                    return failure;
                }
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

// A small wrapper representing the lazy multiplication
// of two coefficients.
template <typename C1, typename C2, typename CR>
struct poly_cf_mul_expr {
    // The two factors.
    const C1 &c1;
    const C2 &c2;

    // Conversion operator to compute
    // and fetch the result of the multiplication.
    explicit operator CR() const
    {
        return c1 * c2;
    }
};

// Simple poly mult implementation: just multiply
// term by term, no parallelisation, no segmentation,
// no tiling, no copying of the operands, etc.
template <typename Ret, typename T, typename U>
inline void poly_mul_impl_simple(Ret &retval, const T &x, const U &y)
{
    using ret_key_t = series_key_t<Ret>;
    using ret_cf_t = series_cf_t<Ret>;
    using cf1_t = series_cf_t<T>;
    using cf2_t = series_cf_t<U>;

    assert(retval.get_symbol_set() == x.get_symbol_set());
    assert(retval.get_symbol_set() == y.get_symbol_set());
    assert(retval.empty());
    assert(retval._get_s_table().size() == 1u);

    // Cache the symbol set.
    const auto &ss = retval.get_symbol_set();

    // Do the monomial overflow checking.
    const bool overflow = !::piranha::monomial_range_overflow_check(
        ::piranha::detail::make_range(::boost::make_transform_iterator(x.begin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(x.end(), poly_term_key_ref_extractor{})),
        ::piranha::detail::make_range(::boost::make_transform_iterator(y.begin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(y.end(), poly_term_key_ref_extractor{})),
        ss);
    if (piranha_unlikely(overflow)) {
        piranha_throw(
            ::std::overflow_error,
            "An overflow in the monomial exponents was detected while attempting to multiply two polynomials");
    }

    // Proceed with the multiplication.
    auto &tab = retval._get_s_table()[0];

    try {
        // Proceed with the multiplication.
        ret_key_t tmp_key;
        for (const auto &[k1, c1] : x) {
            for (const auto &[k2, c2] : y) {
                // Multiply the monomial.
                ::piranha::monomial_mul(tmp_key, k1, k2, ss);

                // Try to insert the new term.
                const auto res = tab.try_emplace(tmp_key, poly_cf_mul_expr<cf1_t, cf2_t, ret_cf_t>{c1, c2});

                // NOTE: optimise with likely/unlikely here?
                if (!res.second) {
                    // The insertion failed, accumulate c1*c2 into the
                    // existing coefficient.
                    // NOTE: do it with fma3(), if possible.
                    if constexpr (is_mult_addable_v<ret_cf_t &, const cf1_t &, const cf2_t &>) {
                        ::piranha::fma3(res.first->second, c1, c2);
                    } else {
                        res.first->second += c1 * c2;
                    }
                }
            }
        }

        // Determine and remove the keys whose coefficients are zero
        // in the return value.
        ::std::vector<ret_key_t> key_remove;
        for (const auto &[k, c] : tab) {
            if (piranha_unlikely(::piranha::is_zero(c))) {
                key_remove.push_back(k);
            }
        }
        for (const auto &k : key_remove) {
            assert(tab.find(k) != tab.end());
            tab.erase(k);
        }
    } catch (...) {
        // retval may now contain zero coefficients.
        // Make sure to clear it before rethrowing.
        tab.clear();
        throw;
    }
}

// Implementation of poly multiplication with identical symbol sets.
template <typename T, typename U>
inline auto poly_mul_impl(T &&x, U &&y)
{
    // Fetch the return type and related types.
    using ret_t = poly_mul_ret_t<T &&, U &&>;
    // using ret_cf_t = series_cf_t<ret_t>;
    // using ret_key_t = series_key_t<ret_t>;

    // Shortcuts to the original types.
    // using rT = remove_cvref_t<T>;
    // using rU = remove_cvref_t<U>;
    // using cf1_t = series_cf_t<rT>;
    // using cf2_t = series_cf_t<rU>;

    assert(x.get_symbol_set() == y.get_symbol_set());
    // const auto &ss = x.get_symbol_set();

    // Create vectors containing copies of
    // the input terms.
    // NOTE: in theory, it would be possible here
    // to move the coefficients (in conjunction with
    // rref_cleaner, as usual).
    // NOTE: perhaps the copying should be avoided
    // if we have small enough series (just operate
    // directly on the original iterators). Keep it in
    // mind for possible future tuning.
    // auto p_transform = [](const auto &p) { return ::std::make_pair(p.first, p.second); };
    // ::std::vector<::std::pair<series_key_t<rT>, cf1_t>> v1(::boost::make_transform_iterator(x.cbegin(), p_transform),
    //                                                        ::boost::make_transform_iterator(x.cend(), p_transform));
    // ::std::vector<::std::pair<series_key_t<rU>, cf2_t>> v2(::boost::make_transform_iterator(y.cbegin(), p_transform),
    //                                                        ::boost::make_transform_iterator(y.cend(), p_transform));

    // // Do the monomial overflow checking.
    // const bool overflow = !::piranha::monomial_range_overflow_check(
    //     ::piranha::detail::make_range(::boost::make_transform_iterator(v1.cbegin(), poly_term_key_ref_extractor{}),
    //                                   ::boost::make_transform_iterator(v1.cend(), poly_term_key_ref_extractor{})),
    //     ::piranha::detail::make_range(::boost::make_transform_iterator(v2.cbegin(), poly_term_key_ref_extractor{}),
    //                                   ::boost::make_transform_iterator(v2.cend(), poly_term_key_ref_extractor{})),
    //     ss);
    // if (piranha_unlikely(overflow)) {
    //     piranha_throw(
    //         ::std::overflow_error,
    //         "An overflow in the monomial exponents was detected while attempting to multiply two polynomials");
    // }

    // Init the return value.
    ret_t retval;
    retval.set_symbol_set(x.get_symbol_set());

    poly_mul_impl_simple(retval, x, y);

    return retval;
}

} // namespace detail

template <typename T, typename U, ::std::enable_if_t<detail::poly_mul_algo<T &&, U &&> != 0, int> = 0>
constexpr detail::poly_mul_ret_t<T &&, U &&> series_mul(T &&x, U &&y)
{
    if (x.get_symbol_set() == y.get_symbol_set()) {
        return detail::poly_mul_impl(::std::forward<T>(x), ::std::forward<U>(y));
    } else {
        using rT = remove_cvref_t<T>;
        using rU = remove_cvref_t<U>;

        // Merge the symbol sets.
        const auto &[merged_ss, ins_map_x, ins_map_y]
            = ::piranha::detail::merge_symbol_sets(x.get_symbol_set(), y.get_symbol_set());

        // The insertion maps cannot be both empty, as we already handled
        // the identical symbol sets case above.
        assert(!ins_map_x.empty() || !ins_map_y.empty());

        // Create a flag indicating empty insertion maps:
        // - 0 -> both non-empty,
        // - 1 -> x is empty,
        // - 2 -> y is empty.
        // (Cannot both be empty as we handled identical symbol sets already).
        const auto flag = static_cast<unsigned>(ins_map_x.empty()) + (static_cast<unsigned>(ins_map_y.empty()) << 1);

        switch (flag) {
            case 1u: {
                // x already has the correct symbol
                // set, extend only y.
                rU b;
                b.set_symbol_set(merged_ss);
                ::piranha::detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

                return detail::poly_mul_impl(::std::forward<T>(x), ::std::move(b));
            }
            case 2u: {
                // y already has the correct symbol
                // set, extend only x.
                rT a;
                a.set_symbol_set(merged_ss);
                ::piranha::detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);

                return detail::poly_mul_impl(::std::move(a), ::std::forward<U>(y));
            }
        }

        // Both x and y need to be extended.
        rT a;
        rU b;
        a.set_symbol_set(merged_ss);
        b.set_symbol_set(merged_ss);
        ::piranha::detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);
        ::piranha::detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

        return detail::poly_mul_impl(::std::move(a), ::std::move(b));
    }
}

} // namespace polynomials

// Lift to the piranha namespace.
template <typename C, typename K>
using polynomial = polynomials::polynomial<C, K>;

// TODO constrain T.
// TODO extra requirements on cf/key?
// TODO make functors.
template <typename T>
inline T make_polynomial(const ::std::string &s, const symbol_set &ss = symbol_set{})
{
    if (ss.empty()) {
        T retval;
        retval.set_symbol_set(symbol_set{s});

        constexpr int arr[] = {1};

        retval.add_term(series_key_t<T>(&arr[0], &arr[0] + 1), 1);

        return retval;
    } else {
        T retval;
        retval.set_symbol_set(ss);

        PIRANHA_MAYBE_TLS ::std::vector<int> tmp;
        tmp.clear();
        tmp.reserve(static_cast<decltype(tmp.size())>(ss.size()));

        bool s_found = false;
        for (const auto &n : ss) {
            if (n == s) {
                s_found = true;
                tmp.push_back(1);
            } else {
                tmp.push_back(0);
            }
        }
        if (piranha_unlikely(!s_found)) {
            // TODO error message.
            piranha_throw(::std::invalid_argument, "");
        }

        retval.add_term(
            series_key_t<T>(static_cast<const int *>(tmp.data()), static_cast<const int *>(tmp.data() + tmp.size())),
            1);

        return retval;
    }
}

namespace detail
{

template <typename T>
inline auto make_polynomials_impl(const ::std::tuple<> &)
{
    return ::std::array<T, 0>{};
}

template <typename T, typename Tuple, ::std::size_t... I>
inline auto make_polynomials_impl_with_ss(const Tuple &t, ::std::index_sequence<I...>)
    PIRANHA_SS_FORWARD_FUNCTION((::std::array<T, ::std::tuple_size_v<Tuple> - 1u>{
        ::piranha::make_polynomial<T>(::std::get<I>(t), ::std::get<::std::tuple_size_v<Tuple> - 1u>(t))...}));

template <typename T, typename Arg0, typename... Args,
          ::std::enable_if_t<::std::is_same_v<::std::tuple_element_t<sizeof...(Args), ::std::tuple<Arg0, Args...>>,
                                              const symbol_set &>,
                             int> = 0>
inline auto make_polynomials_impl(const ::std::tuple<Arg0, Args...> &t) PIRANHA_SS_FORWARD_FUNCTION(
    detail::make_polynomials_impl_with_ss<T>(t, ::std::make_index_sequence<sizeof...(Args)>{}));

template <typename T, typename Tuple, ::std::size_t... I>
inline auto make_polynomials_impl_no_ss(const Tuple &t, ::std::index_sequence<I...>)
    PIRANHA_SS_FORWARD_FUNCTION((::std::array<T, ::std::tuple_size_v<Tuple>>{
        ::piranha::make_polynomial<T>(::std::get<I>(t))...}));

template <typename T, typename Arg0, typename... Args,
          ::std::enable_if_t<!::std::is_same_v<::std::tuple_element_t<sizeof...(Args), ::std::tuple<Arg0, Args...>>,
                                               const symbol_set &>,
                             int> = 0>
inline auto make_polynomials_impl(const ::std::tuple<Arg0, Args...> &t) PIRANHA_SS_FORWARD_FUNCTION(
    detail::make_polynomials_impl_no_ss<T>(t, ::std::make_index_sequence<sizeof...(Args) + 1u>{}));

} // namespace detail

template <typename T, typename... Args>
inline auto make_polynomials(const Args &... args)
    PIRANHA_SS_FORWARD_FUNCTION(detail::make_polynomials_impl<T>(::std::forward_as_tuple(args...)));

} // namespace piranha

#endif
