// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POLYNOMIALS_POLYNOMIAL_HPP
#define OBAKE_POLYNOMIALS_POLYNOMIAL_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/container/container_fwd.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_reduce.h>
#include <tbb/parallel_sort.h>

#include <mp++/integer.hpp>

#include <obake/byte_size.hpp>
#include <obake/config.hpp>
#include <obake/detail/abseil.hpp>
#include <obake/detail/hc.hpp>
#include <obake/detail/ignore.hpp>
#include <obake/detail/it_diff_check.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/type_c.hpp>
#include <obake/detail/xoroshiro128_plus.hpp>
#include <obake/exceptions.hpp>
#include <obake/hash.hpp>
#include <obake/key/key_merge_symbols.hpp>
#include <obake/math/diff.hpp>
#include <obake/math/fma3.hpp>
#include <obake/math/is_zero.hpp>
#include <obake/math/pow.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/math/subs.hpp>
#include <obake/polynomials/monomial_diff.hpp>
#include <obake/polynomials/monomial_homomorphic_hash.hpp>
#include <obake/polynomials/monomial_integrate.hpp>
#include <obake/polynomials/monomial_mul.hpp>
#include <obake/polynomials/monomial_pow.hpp>
#include <obake/polynomials/monomial_range_overflow_check.hpp>
#include <obake/polynomials/monomial_subs.hpp>
#include <obake/ranges.hpp>
#include <obake/series.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace polynomials
{

// The polynomial tag.
struct tag {
};

} // namespace polynomials

template <typename K, typename C>
using polynomial = series<K, C, polynomials::tag>;

namespace detail
{

template <typename T>
struct is_polynomial_impl : ::std::false_type {
};

template <typename K, typename C>
struct is_polynomial_impl<polynomial<K, C>> : ::std::true_type {
};

} // namespace detail

// Detect polynomials.
template <typename T>
using is_polynomial = detail::is_polynomial_impl<T>;

template <typename T>
inline constexpr bool is_polynomial_v = is_polynomial<T>::value;

#if defined(OBAKE_HAVE_CONCEPTS)

template <typename T>
OBAKE_CONCEPT_DECL Polynomial = is_polynomial_v<T>;

#endif

namespace detail
{

// Enabler for make_polynomials():
// - T must be a polynomial,
// - std::string can be constructed from each input Args,
// - poly key can be constructed from a const int * range,
// - poly cf can be constructed from an integral literal.
template <typename T, typename... Args>
using make_polynomials_supported
    = ::std::conjunction<is_polynomial<T>, ::std::is_constructible<::std::string, const Args &>...,
                         ::std::is_constructible<series_key_t<T>, const int *, const int *>,
                         ::std::is_constructible<series_cf_t<T>, int>>;

template <typename T, typename... Args>
using make_polynomials_enabler = ::std::enable_if_t<make_polynomials_supported<T, Args...>::value, int>;

// Overload with a symbol set.
template <typename T, typename... Args, make_polynomials_enabler<T, Args...> = 0>
inline ::std::array<T, sizeof...(Args)> make_polynomials_impl(const symbol_set &ss, const Args &... names)
{
    // Create a temp vector of ints which we will use to
    // init the keys.
    ::std::vector<int> tmp(::obake::safe_cast<::std::vector<int>::size_type>(ss.size()));

    [[maybe_unused]] auto make_poly = [&ss, &tmp](const auto &n) {
        using str_t = remove_cvref_t<decltype(n)>;

        // Fetch a const reference to either the original
        // std::string object n, or to a string temporary
        // created from it.
        const auto &s = [&n]() -> decltype(auto) {
            if constexpr (::std::is_same_v<str_t, ::std::string>) {
                return n;
            } else {
                return ::std::string(n);
            }
        }();

        // Init the retval, assign the symbol set.
        T retval;
        retval.set_symbol_set(ss);

        // Try to locate s within the symbol set.
        const auto it = ss.find(s);
        if (obake_unlikely(it == ss.end() || *it != s)) {
            obake_throw(::std::invalid_argument, "Cannot create a polynomial with symbol set " + detail::to_string(ss)
                                                     + " from the generator '" + s
                                                     + "': the generator is not in the symbol set");
        }

        // Set to 1 the exponent of the corresponding generator.
        tmp[static_cast<::std::vector<int>::size_type>(ss.index_of(it))] = 1;

        // Create and add a new term.
        // NOTE: at least for some monomial types (e.g., packed monomial),
        // we will be computing the iterator difference when constructing from
        // a range. Make sure we can safely represent the size of tmp via
        // iterator difference.
        ::obake::detail::it_diff_check<decltype(::std::as_const(tmp).data())>(tmp.size());
        retval.add_term(series_key_t<T>(::std::as_const(tmp).data(), ::std::as_const(tmp).data() + tmp.size()), 1);

        // Set back to zero the exponent that was previously set to 1.
        tmp[static_cast<::std::vector<int>::size_type>(ss.index_of(it))] = 0;

        return retval;
    };

    return ::std::array<T, sizeof...(Args)>{{make_poly(names)...}};
}

// Overload without a symbol set.
template <typename T, typename... Args, make_polynomials_enabler<T, Args...> = 0>
inline ::std::array<T, sizeof...(Args)> make_polynomials_impl(const Args &... names)
{
    [[maybe_unused]] auto make_poly = [](const auto &n) {
        using str_t = remove_cvref_t<decltype(n)>;

        // Init the retval, assign a symbol set containing only n.
        T retval;
        if constexpr (::std::is_same_v<str_t, ::std::string>) {
            retval.set_symbol_set(symbol_set{n});
        } else {
            retval.set_symbol_set(symbol_set{::std::string(n)});
        }

        constexpr int arr[] = {1};

        // Create and add a new term.
        retval.add_term(series_key_t<T>(&arr[0], &arr[0] + 1), 1);

        return retval;
    };

    return ::std::array<T, sizeof...(Args)>{{make_poly(names)...}};
}

} // namespace detail

#if defined(OBAKE_MSVC_LAMBDA_WORKAROUND)

template <typename T>
struct make_polynomials_msvc {
    template <typename... Args>
    constexpr auto operator()(const Args &... args) const
        OBAKE_SS_FORWARD_MEMBER_FUNCTION(detail::make_polynomials_impl<T>(args...))
};

template <typename T>
inline constexpr auto make_polynomials = make_polynomials_msvc<T>{};

#else

// Polynomial creation functor.
template <typename T>
inline constexpr auto make_polynomials
    = [](const auto &... args) OBAKE_SS_FORWARD_LAMBDA(detail::make_polynomials_impl<T>(args...));

#endif

namespace polynomials
{

namespace detail
{

// Small helper to create a vector of indices
// into the input vector v, in parallel using TBB.
template <typename V>
inline auto poly_mul_impl_par_make_idx_vector(const V &v)
{
    // NOTE: we could use here a custom allocator
    // that does default initialisation, instead of
    // value initialisation.
    ::std::vector<decltype(v.size())> ret;
    ret.resize(::obake::safe_cast<decltype(ret.size())>(v.size()));

    ::tbb::parallel_for(::tbb::blocked_range<decltype(v.size())>(0, v.size()), [&ret](const auto &range) {
        for (auto i = range.begin(); i != range.end(); ++i) {
            ret[i] = i;
        }
    });

    return ret;
}

// Helper to prepare the variables that will hold the degree
// data used during polynomial multiplication. In untruncated
// multiplication, an empty tuple will be returned, otherwise
// a tuple of 2 vectors containing the (partial) degrees of the terms
// in the input series will be returned.
// The input series are of types T and U, while the terms
// of the series are stored in the input vectors v1 and v2.
// It is expected that the term degrees are computed via
// the facilities from series.hpp.
// NOTE: this function does not do any degree computation,
// it just prepares variables of the correct type to hold
// the degrees.
template <typename T, typename U, typename V1, typename V2, typename... Args>
inline auto poly_mul_impl_prepare_degree_data([[maybe_unused]] const V1 &v1, [[maybe_unused]] const V2 &v2,
                                              [[maybe_unused]] const symbol_set &ss,
                                              [[maybe_unused]] const Args &... args)
{
    if constexpr (sizeof...(Args) == 0u) {
        // Untruncated case, return an empty tuple.
        return ::std::make_tuple();
    } else if constexpr (sizeof...(Args) == 1u) {
        // Total truncation.
        return ::std::make_tuple(
            // NOTE: the invocation here uses parallel mode, but it does not really
            // matter as the return type does not change wrt serial mode.
            decltype(customisation::internal::make_degree_vector<T>(v1.cbegin(), v1.cend(), ss, true)){},
            decltype(customisation::internal::make_degree_vector<U>(v2.cbegin(), v2.cend(), ss, true)){});
    } else {
        // Partial truncation.
        return ::std::make_tuple(
            decltype(customisation::internal::make_p_degree_vector<T>(
                v1.cbegin(), v1.cend(), ss, ::std::get<1>(::std::forward_as_tuple(args...)), true)){},
            decltype(customisation::internal::make_p_degree_vector<U>(
                v2.cbegin(), v2.cend(), ss, ::std::get<1>(::std::forward_as_tuple(args...)), true)){});
    }
}

// Small helper to extract a const reference to
// a term's key (that is, the first element of the
// input pair p).
struct poly_term_key_ref_extractor {
    // Reference overload.
    template <typename P>
    constexpr const typename P::first_type &operator()(const P &p) const noexcept
    {
        return p.first;
    }
    // Pointer overload.
    template <typename P>
    constexpr const typename P::first_type &operator()(const P *p) const noexcept
    {
        return p->first;
    }
};

// Meta-programming for selecting the algorithm and the return
// type of polynomial multiplication.
template <typename T, typename U>
constexpr auto poly_mul_algorithm_impl()
{
    // Shortcut for signalling that the mul implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, ::obake::detail::type_c<void>{});

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
            // Determine if the cf/key types support all the necessary
            // bits.
            using cf1_t = series_cf_t<rT>;
            using cf2_t = series_cf_t<rU>;
            using ret_cf_t = detected_t<::obake::detail::mul_t, const cf1_t &, const cf2_t &>;

            if constexpr (::std::conjunction_v<
                              // NOTE: this will also check that switching around the
                              // operands produces the same result. This is necessary
                              // because we might need to switch the arguments
                              // in the top level poly multiplication function.
                              // NOTE: this also ensures that ret_cf_t is detected.
                              is_multipliable<const cf1_t &, const cf2_t &>,
                              // If ret_cf_t a coefficient type?
                              is_cf<ret_cf_t>,
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
                return ::std::make_pair(1, ::obake::detail::type_c<polynomial<series_key_t<rT>, ret_cf_t>>{});
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

// Helper to estimate the average term size (in bytes) in a poly multiplication.
// NOTE: should this also be made proportional to the number
// of estimated term-by-term multiplications?
template <typename RetCf, typename T1, typename T2>
inline ::std::size_t poly_mul_impl_estimate_average_term_size(const ::std::vector<T1> &v1, const ::std::vector<T2> &v2,
                                                              const symbol_set &ss)
{
    using ret_key_t = typename T1::first_type;
    static_assert(::std::is_same_v<ret_key_t, typename T2::first_type>);

    // Compute the padding in the term class.
    constexpr auto pad_size = sizeof(series_term_t<polynomial<ret_key_t, RetCf>>) - (sizeof(RetCf) + sizeof(ret_key_t));

    // Preconditions.
    assert(!v1.empty());
    assert(!v2.empty());

    // Init a xoroshiro rng, with some compile-time
    // randomness mixed in with the sizes of v1/v2.
    constexpr ::std::uint64_t s1 = 18379758338774109289ull;
    constexpr ::std::uint64_t s2 = 15967298767098049689ull;
    ::obake::detail::xoroshiro128_plus rng{static_cast<::std::uint64_t>(s1 + v1.size()),
                                           static_cast<::std::uint64_t>(s2 + v2.size())};

    // The idea now is to compute a small amount of term-by-term
    // multiplications and determine the average size in bytes
    // of the produced terms.
    constexpr auto ntrials = 10u;

    // Temporary monomial used for term-by-term multiplications.
    ret_key_t tmp_key(ss);

    // Create the distributions.
    ::std::uniform_int_distribution<decltype(v1.size())> dist1(0, v1.size() - 1u);
    ::std::uniform_int_distribution<decltype(v2.size())> dist2(0, v2.size() - 1u);

    // Run the trials.
    ::std::size_t acc = 0;
    for (auto i = 0u; i < ntrials; ++i) {
        // Pick a random term in each series.
        const auto idx1 = dist1(rng);
        const auto idx2 = dist2(rng);

        // Multiply monomial and coefficient.
        ::obake::monomial_mul(tmp_key, v1[idx1].first, v2[idx2].first, ss);
        const auto tmp_cf = v1[idx1].second * v2[idx2].second;

        // Accumulate the size of the produced term: size of monomial,
        // coefficient, and, if present, padding.
        acc += ::obake::byte_size(::std::as_const(tmp_key)) + ::obake::byte_size(tmp_cf) + pad_size;
    }

    // Compute the ceil of the average term size.
    const auto ret = acc / ntrials + static_cast<::std::size_t>(acc % ntrials != 0u);

    // NOTE: theoretically ret could be zero if
    // in the loop above we somehow overflow std::size_t.
    // Thus, make it 1 in such a case.
    return ret + static_cast<::std::size_t>(ret == 0u);
}

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(push)
#pragma warning(disable : 4334)

#endif

// Functor to return a copy of the input
// term p, but with the const removed
// from the key type.
struct poly_mul_impl_pair_transform {
    template <typename T>
    auto operator()(const T &p) const
    {
        return ::std::make_pair(p.first, p.second);
    }
};

// This function will:
// - estimate the size of the product of two input polynomials,
// - compute the total number of term-by-term multiplications that will
//   be performed in the computation of the polynomial product
//   (which could be different from the product of the sizes of the
//   factors due to truncation).
// S1 and S2 are the types of the polynomials, x and y the polynomials
// represented as vectors of terms. The extra arguments represent
// the truncation limits.
// Requires x and y not empty, y not shorter than x. The returned
// value is guaranteed to be nonzero.
// NOTE: by imposing that x is the shorter series, we are able to
// greatly reduce the estimation overhead for highly rectangular
// multiplications. The downside is that we overestimate the final
// series size quite a bit. Not sure how we could proceed to
// improve the situation.
template <typename S1, typename S2, typename T1, typename T2, typename... Args>
inline auto poly_mul_estimate_product_size(const ::std::vector<T1> &x, const ::std::vector<T2> &y, const symbol_set &ss,
                                           const Args &... args)
{
    // Preconditions.
    assert(!x.empty());
    assert(!y.empty());
    assert(x.size() <= y.size());
    static_assert(sizeof...(args) <= 2u);

    // Make sure that the input types are consistent.
    using key_type = typename T1::first_type;
    static_assert(::std::is_same_v<key_type, typename T2::first_type>);
    static_assert(::std::is_same_v<series_key_t<S1>, key_type>);
    static_assert(::std::is_same_v<series_key_t<S2>, key_type>);
    static_assert(::std::is_same_v<series_cf_t<S1>, typename T1::second_type>);
    static_assert(::std::is_same_v<series_cf_t<S2>, typename T2::second_type>);

    // Prepare the variable to hold the degree data.
    auto degree_data = detail::poly_mul_impl_prepare_degree_data<S1, S2>(x, y, ss, args...);

    // Prepare vectors of indices into x/y.
    decltype(detail::poly_mul_impl_par_make_idx_vector(x)) vidx1;
    decltype(detail::poly_mul_impl_par_make_idx_vector(y)) vidx2;

    // Concurrently create the degree data for x and y, and fill
    // in the vidx1/vidx2 vectors.
    ::tbb::parallel_invoke(
        [&vidx1, &x, &ss, &degree_data, &args...]() {
            if constexpr (sizeof...(args) == 1u) {
                // Total degree truncation.
                ::obake::detail::ignore(args...);

                ::obake::detail::container_it_diff_check(x);

                ::std::get<0>(degree_data)
                    = customisation::internal::make_degree_vector<S1>(x.cbegin(), x.cend(), ss, true);
            } else if constexpr (sizeof...(args) == 2u) {
                // Partial degree truncation.
                ::obake::detail::container_it_diff_check(x);

                ::std::get<0>(degree_data) = customisation::internal::make_p_degree_vector<S1>(
                    x.cbegin(), x.cend(), ss, ::std::get<1>(::std::forward_as_tuple(args...)), true);
            } else {
                ::obake::detail::ignore(ss, degree_data, args...);
            }

            vidx1 = detail::poly_mul_impl_par_make_idx_vector(x);
        },
        [&vidx2, &y, &ss, &degree_data, &args...]() {
            if constexpr (sizeof...(args) == 1u) {
                // Total degree truncation.
                ::obake::detail::ignore(args...);

                ::obake::detail::container_it_diff_check(y);

                ::std::get<1>(degree_data)
                    = customisation::internal::make_degree_vector<S2>(y.cbegin(), y.cend(), ss, true);
            } else if constexpr (sizeof...(args) == 2u) {
                // Partial degree truncation.
                ::obake::detail::container_it_diff_check(y);

                ::std::get<1>(degree_data) = customisation::internal::make_p_degree_vector<S2>(
                    y.cbegin(), y.cend(), ss, ::std::get<1>(::std::forward_as_tuple(args...)), true);
            } else {
                ::obake::detail::ignore(ss, degree_data, args...);
            }

            vidx2 = detail::poly_mul_impl_par_make_idx_vector(y);

            // In truncated multiplication, order
            // the indices into y according to the degree of
            // the terms, and sort the vector of
            // degrees as well.
            if constexpr (sizeof...(args) > 0u) {
                auto &v2_deg = ::std::get<1>(degree_data);

                ::tbb::parallel_sort(vidx2.begin(), vidx2.end(),
                                     [&cv2_deg = ::std::as_const(v2_deg)](const auto &idx1, const auto &idx2) {
                                         return cv2_deg[idx1] < cv2_deg[idx2];
                                     });

                // Apply the permutation to v2_deg.
                ::obake::detail::container_it_diff_check(v2_deg);
                v2_deg = ::std::remove_reference_t<decltype(v2_deg)>(
                    ::boost::make_permutation_iterator(v2_deg.cbegin(), vidx2.cbegin()),
                    ::boost::make_permutation_iterator(v2_deg.cend(), vidx2.cend()));

                // Verify the sorting in debug mode.
                assert(::std::is_sorted(v2_deg.cbegin(), v2_deg.cend()));
            }
        });

    // Determine the total number of term-by-term multiplications
    // that will be performed in the poly multiplication.
    auto tot_n_mults = [&vidx1, &vidx2, &degree_data, &args...]() {
        if constexpr (sizeof...(Args) == 0u) {
            // In the untruncated case, the total number of term-by-term
            // multiplications to be performed is simply the product
            // of the series sizes.
            ::obake::detail::ignore(degree_data, args...);

            return ::mppp::integer<1>(vidx1.size()) * vidx2.size();
        } else {
            // In the truncated case, we need to take into account
            // the truncation limit term by term.
            ::obake::detail::ignore(vidx2);

            // Fetch the truncation limit.
            const auto &max_deg = ::std::get<0>(::std::forward_as_tuple(args...));

            return ::tbb::parallel_reduce(
                ::tbb::blocked_range<decltype(vidx1.cbegin())>(vidx1.cbegin(), vidx1.cend()), ::mppp::integer<1>{},
                [&max_deg, &degree_data](const auto &range, ::mppp::integer<1> cur) {
                    // NOTE: vidx1 is unsorted, vidx2 is sorted according
                    // to the degree.

                    // Get the degree data for x and y.
                    const auto &[v1_deg, v2_deg] = degree_data;

                    for (auto idx1 : range) {
                        // Fetch the degree of the current term in x.
                        const auto &d1 = v1_deg[idx1];

                        // Find the first degree d2 in v2_deg such that d1 + d2 > max_degree.
                        const auto it = ::std::upper_bound(v2_deg.cbegin(), v2_deg.cend(), max_deg,
                                                           [&d1](const auto &mdeg, const auto &d2) {
                                                               // NOTE: we require below
                                                               // comparability between const lvalue limit
                                                               // and rvalue of the sum of the degrees.
                                                               return mdeg < d1 + d2;
                                                           });

                        // Accumulate in cur how many terms in y would be multiplied
                        // by the current term in x.
                        // NOTE: we checked when constructing v2_deg that its iterator
                        // diff type can represent the total size.
                        cur += it - v2_deg.cbegin();
                    }

                    return cur;
                },
                [](const auto &a, const auto &b) { return a + b; });
        }
    }();

    // Parameters for the random trials.
    // NOTE: the idea here is that the larger the
    // multiplication, the larger the number of trials we can
    // do. Doing more trials also helps stabilizing
    // the estimation. The constant is inferred from
    // testing on the usual benchmarks.
    // NOTE: put a floor of 5 trials, which also ensures
    // that ntrials is never estimated to 0.
    const auto ntrials = ::std::max(5u, ::boost::numeric_cast<unsigned>(5e-8 * tot_n_mults));
    // NOTE: we further divide by 2 below, so that the
    // multiplier is actually 3/2.
    const auto multiplier = 3u;

    // NOTE: workaround for a GCC 7 issue.
    using vidx2_size_t = typename ::std::vector<decltype(y.size())>::size_type;

    // Run the trials.
    // NOTE: ideally, we would like to select without repetition random term-by-term
    // multiplications. This could be done by mapping the two sizes of v1 and v2
    // into a single integer N (k-packing style), and then using a linear congruential
    // generator with period N which guarantees that there are no repetitions within
    // that period. See:
    // https://stackoverflow.com/questions/9755538/how-do-i-create-a-list-of-random-numbers-without-duplicates/53646842
    // https://en.wikipedia.org/wiki/Linear_congruential_generator
    // (see the SO answer a bit down the page).
    // However, I don't know how this would work when truncation is involved.
    // The current approach is to shuffle v1 and then pick randomly into v2.
    // This result in a choice of index in v1 without repetitions, but the
    // random picking in v2 could have repetitions, so it's not precisely
    // equivalent to having truly random term-by-term multiplications.
    auto c_est = ::tbb::parallel_reduce(
        ::tbb::blocked_range<unsigned>(0, ntrials), ::mppp::integer<1>{},
        [multiplier, &degree_data, &x, &y, &vidx1, &vidx2, &ss, &args...](const auto &range, ::mppp::integer<1> cur) {
            // Make a local copy of vidx1.
            auto vidx1_copy(vidx1);

            // Prepare a distribution for randomly indexing into vidx2.
            using dist_type = ::std::uniform_int_distribution<vidx2_size_t>;
            using dist_param_type = typename dist_type::param_type;
            dist_type idist;

            // Init the hash set we will be using for the trials.
            // NOTE: use exactly the same hasher/comparer as in series.hpp, so that
            // we are sure we are being consistent wrt type requirements, etc.
            using local_set = ::absl::flat_hash_set<key_type, ::obake::detail::series_key_hasher,
                                                    ::obake::detail::series_key_comparer>;
            local_set ls;
            ls.reserve(::obake::safe_cast<decltype(ls.size())>(vidx1.size()));

            // Temporary object for monomial multiplications.
            key_type tmp_key(ss);

            for (auto i = range.begin(); i != range.end(); ++i) {
                // Init a random engine for this trial, mixing compile
                // time randomness with the current trial index.
                constexpr ::std::uint64_t s1 = 14295768699618639914ull;
                constexpr ::std::uint64_t s2 = 12042842946850383048ull;
                ::obake::detail::xoroshiro128_plus rng{static_cast<::std::uint64_t>(i + s1),
                                                       static_cast<::std::uint64_t>(i + s2)};

                // Shuffle the indices into the first series.
                ::std::shuffle(vidx1_copy.begin(), vidx1_copy.end(), rng);

                // This will be used to determine the average number of terms in y
                // that participate in the multiplication. It is used only in case
                // there are no collisions at the end of the loop below.
                ::mppp::integer<1> acc_y;

                for (auto idx1 : vidx1_copy) {
                    // Get the upper limit for indexing in vidx2.
                    // NOTE: this will be an index into a vector of indices.
                    const auto limit = [&degree_data, idx1, &vidx2, &args...]() {
                        if constexpr (sizeof...(args) == 0u) {
                            // Untruncated case, just return the size of vidx2.
                            ::obake::detail::ignore(degree_data, idx1);

                            return vidx2.size();
                        } else {
                            // Truncated case: determine the first index
                            // into vidx2 which does not satisfy the truncation
                            // limit.
                            ::obake::detail::ignore(vidx2);

                            // Fetch the truncation limit.
                            const auto &max_deg = ::std::get<0>(::std::forward_as_tuple(args...));

                            // Get the degree data for x and y.
                            const auto &[v1_deg, v2_deg] = degree_data;

                            // Fetch the degree of the current term in x.
                            const auto &d1 = v1_deg[idx1];

                            // Find the first degree d2 in v2_deg such that d1 + d2 > max_degree.
                            const auto it = ::std::upper_bound(v2_deg.cbegin(), v2_deg.cend(), max_deg,
                                                               [&d1](const auto &mdeg, const auto &d2) {
                                                                   // NOTE: we require below
                                                                   // comparability between const lvalue limit
                                                                   // and rvalue of the sum of the degrees.
                                                                   return mdeg < d1 + d2;
                                                               });

                            // We checked when constructing v2_deg that its iterator
                            // diff type can represent the total size. Because
                            // the sizes of vidx2 and v2_deg are the same, the static cast
                            // is also safe.
                            return static_cast<vidx2_size_t>(it - v2_deg.cbegin());
                        }
                    }();

                    if (limit == 0u) {
                        // The upper limit is 0, we cannot multiply by any
                        // term in y without violating the truncation constraint.
                        continue;
                    }

                    // Keep track of how many terms in y would be multiplied
                    // by the current term in x in the full multiplication.
                    acc_y += limit;

                    // Pick a random index in s2 within the limit.
                    const auto idx2 = vidx2[idist(rng, dist_param_type(0u, limit - 1u))];

                    // Try to do the multiplication.
                    ::obake::monomial_mul(tmp_key, x[idx1].first, y[idx2].first, ss);

                    // Try the insertion into the local set.
                    const auto ret = ls.insert(tmp_key);
                    if (!ret.second) {
                        // The key already exists, break out.
                        break;
                    }
                }

                // Determine how many unique terms were generated
                // in the loop above.
                const auto count = ls.size();

                if (count == vidx1_copy.size()) {
                    // We generated as many unique terms as
                    // the number of terms in x. This means that
                    // we will estimate a perfect sparsity. In untruncated
                    // multiplication, this means nx * ny, in a truncated
                    // multiplication is less than that (depending on
                    // how many terms were skipped due to the truncation
                    // limits).
                    cur += acc_y;
                } else {
                    // We detected a duplicate term, use the
                    // quadratic estimate.
                    cur += (::mppp::integer<1>{multiplier} * count * count) >> 1;
                }

                // Clear up the local set for the next iteration.
                ls.clear();
            }

            // Return the accumulated estimate.
            return cur;
        },
        [](const auto &a, const auto &b) { return a + b; });

    // Return the average of the estimates (but don't return zero).
    // NOTE: ntrials is guaranteed to be nonzero from above.
    auto ret = c_est / ntrials;
    if (ret.is_zero()) {
        return ::std::make_tuple(::mppp::integer<1>{1}, ::std::move(tot_n_mults));
    } else {
        return ::std::make_tuple(::std::move(ret), ::std::move(tot_n_mults));
    }
}

// The multi-threaded homomorphic implementation.
template <typename Ret, typename T, typename U, typename... Args>
inline void poly_mul_impl_mt_hm(Ret &retval, const T &x, const U &y, const Args &... args)
{
    using cf1_t = series_cf_t<T>;
    using cf2_t = series_cf_t<U>;
    using ret_key_t = series_key_t<Ret>;
    using ret_cf_t = series_cf_t<Ret>;
    using s_size_t = typename Ret::s_size_type;

    // Preconditions.
    static_assert(sizeof...(args) <= 2u);
    assert(!x.empty());
    assert(!y.empty());
    assert(x.size() <= y.size());
    assert(retval.get_symbol_set() == x.get_symbol_set());
    assert(retval.get_symbol_set() == y.get_symbol_set());
    assert(retval.empty());
    assert(retval._get_s_table().size() == 1u);

    // Cache the symbol set.
    const auto &ss = retval.get_symbol_set();

    // Create vectors containing copies of
    // the input terms.
    // NOTE: in theory, it would be possible here
    // to move the coefficients (in conjunction with
    // rref_cleaner, as usual).
    // NOTE: drop the const from the key type in order
    // to allow mutability.
    // NOTE: need to better assess the benefits of
    // copying the input series.
    ::std::vector<::std::pair<series_key_t<T>, cf1_t>> v1(
        ::boost::make_transform_iterator(x.begin(), poly_mul_impl_pair_transform{}),
        ::boost::make_transform_iterator(x.end(), poly_mul_impl_pair_transform{}));
    ::std::vector<::std::pair<series_key_t<U>, cf2_t>> v2(
        ::boost::make_transform_iterator(y.begin(), poly_mul_impl_pair_transform{}),
        ::boost::make_transform_iterator(y.end(), poly_mul_impl_pair_transform{}));

    // Do the monomial overflow checking, if supported.
    // NOTE: we have to sequence the overflow checking before the product
    // size estimation and the average term size estimation, as those two
    // operations might generate overflows during monomial multiplication.
    const auto r1
        = ::obake::detail::make_range(::boost::make_transform_iterator(v1.cbegin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(v1.cend(), poly_term_key_ref_extractor{}));
    const auto r2
        = ::obake::detail::make_range(::boost::make_transform_iterator(v2.cbegin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(v2.cend(), poly_term_key_ref_extractor{}));
    if constexpr (are_overflow_testable_monomial_ranges_v<decltype(r1) &, decltype(r2) &>) {
        // The monomial overflow checking is supported, run it.
        if (obake_unlikely(!::obake::monomial_range_overflow_check(r1, r2, ss))) {
            obake_throw(::std::overflow_error, "An overflow in the monomial exponents was detected while "
                                               "attempting to multiply two polynomials");
        }
    }

    // Estimate the total number of terms, and compute the total number
    // of term-by-term multiplications.
    // NOTE: poly_mul_estimate_product_size() requires the shorter series first,
    // which is ensured by the preconditions of this function.
    const auto [est_nterms, tot_n_mults] = detail::poly_mul_estimate_product_size<T, U>(v1, v2, ss, args...);
    // Exit early if the truncation limits
    // result in an empty output series.
    if (sizeof...(Args) > 0u && tot_n_mults.is_zero()) {
        return;
    }

    // Estimate the average term size.
    // NOTE: once poly_mul_impl_estimate_average_term_size() becomes more computationally intensive,
    // we can do it in parallel with poly_mul_estimate_product_size().
    const auto avg_term_size = detail::poly_mul_impl_estimate_average_term_size<ret_cf_t>(v1, v2, ss);

    // Compute the estimated sparsity.
    const auto est_sp = static_cast<double>(est_nterms) / static_cast<double>(tot_n_mults);

    // Establish the desired segment size in kilobytes.
    // NOTE: the idea here is the following. For highly
    // sparse polynomials (est_sp >= threshold), we want to pick
    // a relatively large size so that it fits somewhere in L2
    // cache, say. The reason is that we won't do much computation
    // per segment due to the sparsity, thus we aim at reducing
    // the parallelisation overhead by operating on larger chunks
    // of the product series. When the sparsity is smaller, then
    // we have a higher computational density, thus we will spend
    // more time computing a single segment, and thus we can aim
    // at staying in L1 cache instead, as the parallelisation overhead
    // will be smaller. Note that these values will be just rule-of-thumb,
    // because the sparsity is not estimated accurately and because
    // of further manipulations below. Additionally, it is not clear
    // to me how smooth the transition between high and low sparsity
    // will be. Eventually, we may perhaps want to leave this parameter as
    // a tunable parameter for the user or perhaps even determine
    // the cache sizes at runtime and use those.
    // NOTE: if est_sp is not finite, due to tot_n_mults being zero or other
    // FP issues, go with a default value.
    // NOTE: is it worth it to exit early if tot_n_mults is zero? This would
    // mean that the truncation limits will produce an empty series.
    const auto seg_size = (!::std::isfinite(est_sp) || est_sp >= 1E-3) ? 200ul : 20ul;

    // Estimate the number of segments via the deduced segment size.
    const auto est_nsegs = (est_nterms * avg_term_size) / (seg_size * 1024ul);

    // Fetch the base-2 logarithm + 1 of est_nsegs, making sure it does not
    // overflow the max allowed value for the return polynomial type.
    const auto log2_nsegs = ::std::min(::obake::safe_cast<unsigned>(est_nsegs.nbits()),
                                       polynomial<ret_key_t, ret_cf_t>::get_max_s_size());

    // Setup the number of segments in retval.
    retval.set_n_segments(log2_nsegs);

    // Cache the actual number of segments.
    const auto nsegs = s_size_t(1) << log2_nsegs;

    // Helper to sort the input terms according to the hash value modulo
    // 2**log2_nsegs. That is, sort them according to the bucket
    // they would occupy in a segmented table with 2**log2_nsegs
    // segments.
    auto t_sorter = [log2_nsegs](const auto &p1, const auto &p2) {
        const auto h1 = ::obake::hash(p1.first);
        const auto h2 = ::obake::hash(p2.first);

        return h1 % (s_size_t(1) << log2_nsegs) < h2 % (s_size_t(1) << log2_nsegs);
    };

    // Helper to compute the segmentation for the input series.
    // The segmentation is a vector of ranges (represented
    // as pairs of indices into v1/v2) paired to indices
    // representing the bucket that the range
    // would occupy in a segmented table
    // with 2**log2_nsegs segments.
    auto compute_vseg = [nsegs, log2_nsegs](const auto &v) {
        // Ensure that the size of v is representable by
        // its iterator's diff type. We need to do some
        // iterator arithmetics below.
        ::obake::detail::container_it_diff_check(v);

        using idx_t = decltype(v.size());
        ::std::vector<::std::tuple<idx_t, idx_t, s_size_t>> vseg;
        // NOTE: the max possible size of vseg is the number of segments.
        vseg.reserve(::obake::safe_cast<decltype(vseg.size())>(nsegs));

        const auto v_begin = v.begin(), v_end = v.end();

        // NOTE: if the number of terms in v is small enough,
        // compute a sparse representation of vseg (meaning that
        // the number of segmentation ranges will be less than
        // nsegs and all the ranges will be non-empty). Otherwise,
        // compute a dense representation, where the number
        // of ranges is equal to nsegs and empty ranges might
        // be present. We will later run different parallel
        // functors depending on whether we are in the sparse
        // or dense case.
        // NOTE: the idea here is that we want to run
        // the sparse functor only in highly sparse cases,
        // because otherwise the more complicated logic
        // of the sparse functor carries a measurable performance
        // penalty in the "mostly-dense" cases.
        if (v.size() < nsegs / 2u) {
            for (auto it = v_begin; it != v_end;) {
                // Get the bucket index of the current term.
                const auto cur_b_idx = static_cast<s_size_t>(::obake::hash(it->first) % (s_size_t(1) << log2_nsegs));
                // Look for the first term whose bucket index is greater than cur_b_idx.
                const auto range_end
                    = ::std::upper_bound(it, v_end, cur_b_idx, [log2_nsegs](const auto &b_idx, const auto &p) {
                          return b_idx < ::obake::hash(p.first) % (s_size_t(1) << log2_nsegs);
                      });
                // NOTE: because we are in the sparse representation case,
                // range_end cannot be equal to it.
                assert(range_end != it);
                // Add the range to vseg.
                // NOTE: the overflow check was done earlier.
                vseg.emplace_back(static_cast<idx_t>(it - v_begin), static_cast<idx_t>(range_end - v_begin), cur_b_idx);
                // Update 'it'.
                it = range_end;
            }
        } else {
            // NOTE: the segmentation in dense form may be
            // parallelised easily if needed.
            idx_t idx = 0;
            auto it = v_begin;
            for (s_size_t i = 0; i < nsegs; ++i) {
                // Look for the first term whose bucket index is greater than i.
                // NOTE: this might result in 'it' not changing, in which case
                // the segmentation range will be empty.
                it = ::std::upper_bound(it, v_end, i, [log2_nsegs](const auto &b_idx, const auto &p) {
                    return b_idx < ::obake::hash(p.first) % (s_size_t(1) << log2_nsegs);
                });
                const auto old_idx = idx;
                // NOTE: the overflow check was done earlier.
                idx = static_cast<idx_t>(it - v_begin);
                vseg.emplace_back(old_idx, idx, i);
            }
        }

        return vseg;
    };

    // Helper that, given a segmentation vseg into a vector of terms
    // v, will:
    //
    // - create and return a vector vd
    //   containing the total/partial degrees of all the
    //   terms in v, with the degrees within each vseg range sorted
    //   in ascending order,
    // - sort v according to vd.
    //
    // v will be one of v1/v2, t is a type_c instance
    // containing either T or U.
    auto seg_sorter = [&ss, &args...](auto &v, auto t, const auto &vseg) {
        if constexpr (sizeof...(args) == 0u) {
            // Non-truncated case: seg_sorter will be a no-op.
            ::obake::detail::ignore(v, t, vseg, ss, args...);
        } else {
            // Truncated case.

            // NOTE: we will be using the machinery from the default implementation
            // of degree() for series, so that we can re-use the concept checking bits
            // as well.
            using s_t = typename decltype(t)::type;

            // Compute the vector of degrees.
            auto vd = [&v, &ss, &args...]() {
                // NOTE: in the make_(p_)degree_vector() helpers we need
                // to compute the size of v via iterator differences.
                ::obake::detail::container_it_diff_check(v);

                if constexpr (sizeof...(args) == 1u) {
                    // Total degree.
                    ::obake::detail::ignore(args...);

                    return customisation::internal::make_degree_vector<s_t>(v.cbegin(), v.cend(), ss, true);
                } else {
                    // Partial degree.
                    static_assert(sizeof...(args) == 2u);

                    return customisation::internal::make_p_degree_vector<s_t>(
                        v.cbegin(), v.cend(), ss, ::std::get<1>(::std::forward_as_tuple(args...)), true);
                }
            }();

            // Ensure that the size of vd is representable by the
            // diff type of its iterators. We'll need to do some
            // iterator arithmetics below.
            // NOTE: cast to const as we will use cbegin/cend below.
            ::obake::detail::container_it_diff_check(::std::as_const(vd));

            // Create a vector of indices into vd.
            auto vidx = detail::poly_mul_impl_par_make_idx_vector(vd);

            // Sort indirectly each range from vseg according to the degree.
            // NOTE: capture vd as const ref because in the lt-comparable requirements for the degree
            // type we are using const lrefs.
            ::tbb::parallel_for(
                ::tbb::blocked_range<decltype(vseg.cbegin())>(vseg.cbegin(), vseg.cend()),
                [&vidx, &vdc = ::std::as_const(vd)](const auto &range) {
                    for (const auto &r : range) {
                        // NOTE: note sure if it is worth to run
                        // a parallel sort here.
                        ::std::sort(vidx.data() + ::std::get<0>(r), vidx.data() + ::std::get<1>(r),
                                    [&vdc](const auto &idx1, const auto &idx2) { return vdc[idx1] < vdc[idx2]; });
                    }
                });

            // Apply the sorting to vd and v. Ensure we don't run
            // into overflows during the permutated access.
            ::obake::detail::container_it_diff_check(vd);
            // NOTE: use cbegin/cend on vd to ensure the copy ctor of
            // the degree type is being called.
            vd = decltype(vd)(::boost::make_permutation_iterator(vd.cbegin(), vidx.cbegin()),
                              ::boost::make_permutation_iterator(vd.cend(), vidx.cend()));
            ::obake::detail::container_it_diff_check(v);
            v = ::std::remove_reference_t<decltype(v)>(::boost::make_permutation_iterator(v.cbegin(), vidx.cbegin()),
                                                       ::boost::make_permutation_iterator(v.cend(), vidx.cend()));

#if !defined(NDEBUG)
            // Check the results in debug mode.
            for (const auto &r : vseg) {
                const auto &idx_begin = ::std::get<0>(r);
                const auto &idx_end = ::std::get<1>(r);

                // NOTE: add constness to vd in order to ensure that
                // the degrees are compared via const refs.
                assert(::std::is_sorted(::std::as_const(vd).data() + idx_begin, ::std::as_const(vd).data() + idx_end));

                if constexpr (sizeof...(args) == 1u) {
                    using d_impl = customisation::internal::series_default_degree_impl;

                    assert(::std::equal(
                        vd.data() + idx_begin, vd.data() + idx_end,
                        ::boost::make_transform_iterator(v.data() + idx_begin, d_impl::d_extractor<s_t>{&ss}),
                        [](const auto &a, const auto &b) { return !(a < b) && !(b < a); }));
                } else {
                    using d_impl = customisation::internal::series_default_p_degree_impl;

                    const auto &s = ::std::get<1>(::std::forward_as_tuple(args...));
                    const auto si = ::obake::detail::ss_intersect_idx(s, ss);

                    assert(::std::equal(
                        vd.data() + idx_begin, vd.data() + idx_end,
                        ::boost::make_transform_iterator(v.data() + idx_begin, d_impl::d_extractor<s_t>{&s, &si, &ss}),
                        [](const auto &a, const auto &b) { return !(a < b) && !(b < a); }));
                }
            }
#endif

            // Return the vector of degrees.
            return vd;
        }
    };

    // Prepare the variables to hold the segmentations
    // and the degrees of the terms, if we are in a
    // truncated multiplication.
    decltype(compute_vseg(v1)) vseg1;
    decltype(compute_vseg(v2)) vseg2;
    auto degree_data = detail::poly_mul_impl_prepare_degree_data<T, U>(v1, v2, ss, args...);

    // For both x and y, concurrently:
    // - sort v1/v2 according to the segmentation order,
    // - compute the segmentation ranges,
    // - compute the degrees of the terms and sort according
    //   to the degree within each segment (only for truncated
    //   multiplication).
    ::tbb::parallel_invoke(
        [&v1, t_sorter, &vseg1, compute_vseg, &degree_data, seg_sorter]() {
            ::tbb::parallel_sort(v1.begin(), v1.end(), t_sorter);
            vseg1 = compute_vseg(v1);
            if constexpr (sizeof...(Args) > 0u) {
                ::std::get<0>(degree_data) = seg_sorter(v1, ::obake::detail::type_c<T>{}, vseg1);
            } else {
                ::obake::detail::ignore(degree_data, seg_sorter);
            }
        },
        [&v2, t_sorter, &vseg2, compute_vseg, &degree_data, seg_sorter]() {
            ::tbb::parallel_sort(v2.begin(), v2.end(), t_sorter);
            vseg2 = compute_vseg(v2);
            if constexpr (sizeof...(Args) > 0u) {
                ::std::get<1>(degree_data) = seg_sorter(v2, ::obake::detail::type_c<U>{}, vseg2);
            } else {
                ::obake::detail::ignore(degree_data, seg_sorter);
            }
        });

#if !defined(NDEBUG)
    {
        // Check the segmentations in debug mode.
        assert(vseg1.size() <= nsegs);
        assert(vseg2.size() <= nsegs);

        auto verify_seg = [log2_nsegs, nsegs](const auto &vs, const auto &v) {
            // Counter for the number of terms represented in the
            // segmentation vector.
            decltype(v.size()) counter = 0;

            for (const auto &t : vs) {
                const auto &[start, end, b_idx] = t;
                assert(end <= v.size());
                // NOTE: different check depending
                // on whether we are in the sparse or
                // dense case.
                if (vs.size() < nsegs) {
                    assert(start < end);
                } else {
                    assert(start <= end);
                }
                counter += end - start;

                // Check that all elements in the range
                // hash to the correct bucket index.
                for (auto idx = start; idx < end; ++idx) {
                    assert(::obake::hash(v[idx].first) % (s_size_t(1) << log2_nsegs) == b_idx);
                }
            }

            assert(counter == v.size());
        };

        verify_seg(vseg1, v1);
        verify_seg(vseg2, v2);
    }
#endif

    // Functor to compute the end index in the
    // inner multiplication loops below, given
    // an index into the first series and a segmentation
    // range into the second series. In non-truncated
    // mode, this functor will always return the end
    // of the range, otherwise the returned
    // value will ensure that the truncation limits
    // are respected.
    auto compute_end_idx2 = [&degree_data, &args...]() {
        if constexpr (sizeof...(Args) == 0u) {
            ::obake::detail::ignore(degree_data, args...);

            return [](const auto &, const auto &r2) { return ::std::get<1>(r2); };
        } else {
            // Create and return the functor. The degree data
            // for the two series will be moved in as vd1 and vd2.
#if defined(_MSC_VER)
            // Until MS fixes the lambda capture.
            auto vd1 = ::std::move(::std::get<0>(degree_data));
            auto vd2 = ::std::move(::std::get<1>(degree_data));
            return [vd1, vd2,
#else

            return [vd1 = ::std::move(::std::get<0>(degree_data)), vd2 = ::std::move(::std::get<1>(degree_data)),
#endif
                    // NOTE: max_deg is captured via const lref this way,
                    // as args is passed as a const lref pack.
                    &max_deg = ::std::get<0>(::std::forward_as_tuple(args...))](const auto &i, const auto &r2) {
                using idx_t = remove_cvref_t<decltype(i)>;

                // Get the total/partial degree of the current term
                // in the first series.
                const auto &d_i = vd1[i];

                // Find the first term in the range r2 such
                // that d_i + d_j > max_deg.
                // NOTE: we checked above that the static cast
                // to the it diff type is safe.
                using it_diff_t = decltype(vd2.cend() - vd2.cbegin());
                const auto it = ::std::upper_bound(vd2.cbegin() + static_cast<it_diff_t>(::std::get<0>(r2)),
                                                   vd2.cbegin() + static_cast<it_diff_t>(::std::get<1>(r2)), max_deg,
                                                   [&d_i](const auto &mdeg, const auto &d_j) {
                                                       // NOTE: we require below
                                                       // comparability between const lvalue limit
                                                       // and rvalue of the sum of the degrees.
                                                       return mdeg < d_i + d_j;
                                                   });

                // Turn the iterator into an index and return it.
                // NOTE: we checked above that the iterator diff
                // type can safely be used as an index (for both
                // vd1 and vd2).
                return static_cast<idx_t>(it - vd2.cbegin());
            };
        }
    }();

#if !defined(NDEBUG)
    // Variable that we use in debug mode to
    // check that all term-by-term multiplications
    // are performed.
    ::std::atomic<unsigned long long> n_mults(0);
#endif

    // The parallel multiplication functor for the sparse case.
    auto sparse_par_functor
        = [&v1, &v2, &vseg1, &vseg2, nsegs, &retval, &ss, mts = retval._get_max_table_size(), &compute_end_idx2
#if !defined(NDEBUG)
           ,
           log2_nsegs, &n_mults
#endif
    ](const auto &range) {
              // Cache the pointers to the terms data.
              // NOTE: doing it here rather than in the lambda
              // capture seems to help performance on GCC.
              auto vptr1 = v1.data();
              auto vptr2 = v2.data();

              // Temporary variable used in monomial multiplication.
              ret_key_t tmp_key(ss);

              // Cache begin/end interators into vseg2.
              const auto vseg2_begin = vseg2.begin(), vseg2_end = vseg2.end();

              for (auto seg_idx = range.begin(); seg_idx != range.end(); ++seg_idx) {
                  // Get a reference to the current table in retval.
                  auto &table = retval._get_s_table()[seg_idx];

                  // The iterator in vseg2 that we will use
                  // as the end point in the binary search below.
                  // Initially, it is just the end of vseg2
                  // (so that all of vseg2 is searched).
                  auto end_search = vseg2_end;

                  // The wrap around flag (see below).
                  bool wrap_around = false;

                  for (const auto &r1 : vseg1) {
                      // Unpack in local variables.
                      const auto [r1_start, r1_end, bi1] = r1;

                      // The first time that bi1 is > seg_idx
                      // we have a wrap-around. This means that:
                      // - the search range in vseg2 will be reset
                      //   to [vseg2_begin, vseg2_end),
                      // - the bucket idx we need to look for
                      //   in vseg2 is not seg_idx any more, but
                      //   seg_idx + nsegs.
                      // E.g., if seg_idx is 4, bi1 is 5 and nsegs
                      // is 8, then there is no bucket index bi2 in
                      // vseg2 such that 5 + bi2 = 4, but there might
                      // be a bi2 such that 5 + bi2 = 4 + 8.
                      if (!wrap_around && bi1 > seg_idx) {
                          wrap_around = true;
                          end_search = vseg2_end;
                      }

                      // Compute the target idx: this is seg_idx in case we
                      // have not wrapped around yet, otherwise seg_idx + nsegs
                      // (so that tgt_idx % nsegs == seg_idx).
                      // NOTE: the guarantee on get_max_s_size() ensures that
                      // we can always compute seg_idx + nsegs without overflow.
                      const auto tgt_idx = wrap_around ? (seg_idx + nsegs) : seg_idx;

                      // Locate a range in vseg2 such that the bucket idx of that range + bi1
                      // is equal to tgt_idx.
                      const auto it = ::std::lower_bound(
                          vseg2_begin, end_search, tgt_idx,
                          [bi1_ = bi1](const auto &t, const auto &b_idx) { return ::std::get<2>(t) + bi1_ < b_idx; });

                      if (it == end_search || ::std::get<2>(*it) + bi1 != tgt_idx) {
                          // There is no range in vseg2 such that its multiplication
                          // by the current range in vseg1 results in terms which
                          // end up at the bucket index seg_idx in the destination
                          // segmented table. Move to the next range in vseg1.
                          continue;
                      }
                      // Update the end point of the binary search. We know that
                      // the next vseg1 range will bump up bi1 at least by one, thus,
                      // in the next binary search, we know that anything we may find
                      // must be *before* it.
                      end_search = it;

                      // Unpack in local variables.
                      const auto &r2 = *it;
                      const auto [r2_start, r2_end, bi2] = r2;
                      ::obake::detail::ignore(r2_end, bi2);

                      // The O(N**2) multiplication loop over the ranges.
                      for (auto idx1 = r1_start; idx1 != r1_end; ++idx1) {
                          const auto &[k1, c1] = *(vptr1 + idx1);

                          // Compute the end index in the second range
                          // for the current value of idx1.
                          const auto idx_end2 = compute_end_idx2(idx1, r2);

                          // In the truncated case, check if the end index
                          // coincides with the begin index. In such a case,
                          // we can skip all the remaining indices in r1 because
                          // none of them will ever generate a term which respects
                          // the truncation limits (both r1 and r2 are sorted
                          // according to the degree).
                          if (sizeof...(Args) > 0u && idx_end2 == r2_start) {
                              break;
                          }

                          const auto end2 = vptr2 + idx_end2;
                          for (auto ptr2 = vptr2 + r2_start; ptr2 != end2; ++ptr2) {
                              const auto &[k2, c2] = *ptr2;

                              // Do the monomial multiplication.
                              ::obake::monomial_mul(tmp_key, k1, k2, ss);

                              // Check that the result ends up in the correct bucket.
                              assert(::obake::hash(tmp_key) % (s_size_t(1) << log2_nsegs) == seg_idx);

                              // Attempt the insertion.
                              // NOTE: this will attempt to insert a term with a default-constructed
                              // coefficient. This is wasteful, it would be better to directly
                              // construct the coefficient product only if the insertion actually
                              // takes place (using a lazy multiplication approach).
                              // Unfortunately, abseil's hash map is not exception safe,
                              // and if the lazy multiplication throws, the table will be left
                              // in an inconsistent state. See:
                              // https://github.com/abseil/abseil-cpp/issues/388
                              // See the commit
                              // 3e334f560d5844f5f2d8face05aa58be21649ff8
                              // for an implementation of the lazy multiplication approach.
                              // If they fix the exception safety issue, we can re-enable the
                              // lazy approach.
                              // NOTE: the coefficient concept demands default constructibility,
                              // thus we can always emplace without arguments for the coefficient.
                              const auto res = table.try_emplace(tmp_key);

                              // NOTE: optimise with likely/unlikely here?
                              if (res.second) {
                                  // NOTE: coefficients are guaranteed to be move-assignable.
                                  res.first->second = c1 * c2;
                              } else {
                                  // The insertion failed, a term with the same monomial
                                  // exists already. Accumulate c1*c2 into the
                                  // existing coefficient.
                                  // NOTE: do it with fma3(), if possible.
                                  if constexpr (is_mult_addable_v<ret_cf_t &, const cf1_t &, const cf2_t &>) {
                                      ::obake::fma3(res.first->second, c1, c2);
                                  } else {
                                      res.first->second += c1 * c2;
                                  }
                              }

#if !defined(NDEBUG)
                              ++n_mults;
#endif
                          }
                      }
                  }

                  // Locate and erase terms with zero coefficients
                  // in the current table.
                  const auto it_f = table.end();
                  for (auto it = table.begin(); it != it_f;) {
                      // NOTE: abseil's flat_hash_map returns void on erase(),
                      // thus we need to increase 'it' before possibly erasing.
                      // erase() does not cause rehash and thus will not invalidate
                      // any other iterator apart from the one being erased.
                      if (obake_unlikely(::obake::is_zero(::std::as_const(it->second)))) {
                          table.erase(it++);
                      } else {
                          ++it;
                      }
                  }

                  // LCOV_EXCL_START
                  // Check the table size against the max allowed size.
                  if (obake_unlikely(table.size() > mts)) {
                      obake_throw(::std::overflow_error, "The homomorphic multithreaded multiplication of two "
                                                         "polynomials resulted in a table whose size ("
                                                             + ::obake::detail::to_string(table.size())
                                                             + ") is larger than the maximum allowed value ("
                                                             + ::obake::detail::to_string(mts) + ")");
                  }
                  // LCOV_EXCL_STOP
              }
          };

    // The parallel multiplication functor for the dense case.
    auto dense_par_functor
        = [&v1, &v2, &vseg1, &vseg2, nsegs, &retval, &ss, mts = retval._get_max_table_size(), &compute_end_idx2
#if !defined(NDEBUG)
           ,
           log2_nsegs, &n_mults
#endif
    ](const auto &range) {
              // Cache the pointers to the terms data.
              // NOTE: doing it here rather than in the lambda
              // capture seems to help performance on GCC.
              auto vptr1 = v1.data();
              auto vptr2 = v2.data();

              // Temporary variable used in monomial multiplication.
              ret_key_t tmp_key(ss);

              for (auto seg_idx = range.begin(); seg_idx != range.end(); ++seg_idx) {
                  // Get a reference to the current table in retval.
                  auto &table = retval._get_s_table()[seg_idx];

                  // The objective here is to perform all term-by-term multiplications
                  // whose results end up in the current table (i.e., the table in retval
                  // at index seg_idx). Due to homomorphic hashing, we know that,
                  // given two indices i and j in vseg1 and vseg2,
                  // the terms generated by the multiplication of the
                  // ranges vseg1[i] and vseg2[j] end up at the bucket
                  // (i + j) % nsegs in retval. Thus, we need to select
                  // all i, j pairs such that (i + j) % nsegs == seg_idx.
                  for (s_size_t i = 0; i < nsegs; ++i) {
                      const auto j = seg_idx >= i ? (seg_idx - i) : (nsegs - i + seg_idx);
                      assert(j < vseg2.size());

                      // Fetch the corresponding ranges.
                      const auto [r1_start, r1_end, bi1] = vseg1[i];
                      const auto &r2 = vseg2[j];
                      const auto [r2_start, r2_end, bi2] = r2;
                      ::obake::detail::ignore(r2_end);

                      // NOTE: in the dense case, the bucket
                      // indices must be equal to i/j.
                      assert(bi1 == i);
                      assert(bi2 == j);
                      ::obake::detail::ignore(bi1);
                      ::obake::detail::ignore(bi2);

                      // The O(N**2) multiplication loop over the ranges.
                      for (auto idx1 = r1_start; idx1 != r1_end; ++idx1) {
                          const auto &[k1, c1] = *(vptr1 + idx1);

                          // Compute the end index in the second range
                          // for the current value of idx1.
                          const auto idx_end2 = compute_end_idx2(idx1, r2);

                          // In the truncated case, check if the end index
                          // coincides with the begin index. In such a case,
                          // we can skip all the remaining indices in r1 because
                          // none of them will ever generate a term which respects
                          // the truncation limits (both r1 and r2 are sorted
                          // according to the degree).
                          if (sizeof...(Args) > 0u && idx_end2 == r2_start) {
                              break;
                          }

                          const auto end2 = vptr2 + idx_end2;
                          for (auto ptr2 = vptr2 + r2_start; ptr2 != end2; ++ptr2) {
                              const auto &[k2, c2] = *ptr2;

                              // Do the monomial multiplication.
                              ::obake::monomial_mul(tmp_key, k1, k2, ss);

                              // Check that the result ends up in the correct bucket.
                              assert(::obake::hash(tmp_key) % (s_size_t(1) << log2_nsegs) == seg_idx);

                              // Attempt the insertion.
                              // NOTE: this will attempt to insert a term with a default-constructed
                              // coefficient. This is wasteful, it would be better to directly
                              // construct the coefficient product only if the insertion actually
                              // takes place (using a lazy multiplication approach).
                              // Unfortunately, abseil's hash map is not exception safe,
                              // and if the lazy multiplication throws, the table will be left
                              // in an inconsistent state. See:
                              // https://github.com/abseil/abseil-cpp/issues/388
                              // See the commit
                              // 3e334f560d5844f5f2d8face05aa58be21649ff8
                              // for an implementation of the lazy multiplication approach.
                              // If they fix the exception safety issue, we can re-enable the
                              // lazy approach.
                              // NOTE: the coefficient concept demands default constructibility,
                              // thus we can always emplace without arguments for the coefficient.
                              const auto res = table.try_emplace(tmp_key);

                              // NOTE: optimise with likely/unlikely here?
                              if (res.second) {
                                  // NOTE: coefficients are guaranteed to be move-assignable.
                                  res.first->second = c1 * c2;
                              } else {
                                  // The insertion failed, a term with the same monomial
                                  // exists already. Accumulate c1*c2 into the
                                  // existing coefficient.
                                  // NOTE: do it with fma3(), if possible.
                                  if constexpr (is_mult_addable_v<ret_cf_t &, const cf1_t &, const cf2_t &>) {
                                      ::obake::fma3(res.first->second, c1, c2);
                                  } else {
                                      res.first->second += c1 * c2;
                                  }
                              }

#if !defined(NDEBUG)
                              ++n_mults;
#endif
                          }
                      }
                  }

                  // Locate and erase terms with zero coefficients
                  // in the current table.
                  const auto it_f = table.end();
                  for (auto it = table.begin(); it != it_f;) {
                      // NOTE: abseil's flat_hash_map returns void on erase(),
                      // thus we need to increase 'it' before possibly erasing.
                      // erase() does not cause rehash and thus will not invalidate
                      // any other iterator apart from the one being erased.
                      if (obake_unlikely(::obake::is_zero(::std::as_const(it->second)))) {
                          table.erase(it++);
                      } else {
                          ++it;
                      }
                  }

                  // LCOV_EXCL_START
                  // Check the table size against the max allowed size.
                  if (obake_unlikely(table.size() > mts)) {
                      obake_throw(::std::overflow_error, "The homomorphic multithreaded multiplication of two "
                                                         "polynomials resulted in a table whose size ("
                                                             + ::obake::detail::to_string(table.size())
                                                             + ") is larger than the maximum allowed value ("
                                                             + ::obake::detail::to_string(mts) + ")");
                  }
                  // LCOV_EXCL_STOP
              }
          };

    try {
        if (vseg1.size() == nsegs && vseg2.size() == nsegs) {
            // Both vseg1 and vseg2 are represented in dense
            // form, run the dense functor.
            ::tbb::parallel_for(::tbb::blocked_range<s_size_t>(0, nsegs), dense_par_functor);
        } else {
            // At least one of vseg1/vseg2 are represented
            // in sparse form, run the sparse functor.
            ::tbb::parallel_for(::tbb::blocked_range<s_size_t>(0, nsegs), sparse_par_functor);
        }

#if !defined(NDEBUG)
        // Verify the number of term multiplications we performed,
        // but only if we are in non-truncated mode.
        if constexpr (sizeof...(args) == 0u) {
            assert(n_mults.load()
                   == static_cast<unsigned long long>(x.size()) * static_cast<unsigned long long>(y.size()));
        }
#endif
        // LCOV_EXCL_START
    } catch (...) {
        // In case of exceptions, clear retval before
        // rethrowing to ensure a known sane state.
        retval.clear();
        throw;
        // LCOV_EXCL_STOP
    }
}

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(pop)

#endif

// Extract a pointer from a const reference.
struct poly_mul_impl_ptr_extractor {
    template <typename T>
    auto operator()(const T &x) const
    {
        return &x;
    }
};

// Simple poly mult implementation: just multiply
// term by term, no parallelisation, no segmentation,
// no copying of the operands, etc.
template <typename Ret, typename T, typename U, typename... Args>
inline void poly_mul_impl_simple(Ret &retval, const T &x, const U &y, const Args &... args)
{
    using ret_key_t = series_key_t<Ret>;
    using ret_cf_t = series_cf_t<Ret>;
    using cf1_t = series_cf_t<T>;
    using cf2_t = series_cf_t<U>;

    // Preconditions.
    static_assert(sizeof...(args) <= 2u);
    assert(!x.empty());
    assert(!y.empty());
    assert(x.size() <= y.size());
    assert(retval.get_symbol_set() == x.get_symbol_set());
    assert(retval.get_symbol_set() == y.get_symbol_set());
    assert(retval.empty());
    assert(retval._get_s_table().size() == 1u);

    // Cache the symbol set.
    const auto &ss = retval.get_symbol_set();

    // Construct the vectors of pointer to the terms.
    ::std::vector<const series_term_t<T> *> v1(
        ::boost::make_transform_iterator(x.begin(), poly_mul_impl_ptr_extractor{}),
        ::boost::make_transform_iterator(x.end(), poly_mul_impl_ptr_extractor{}));
    ::std::vector<const series_term_t<U> *> v2(
        ::boost::make_transform_iterator(y.begin(), poly_mul_impl_ptr_extractor{}),
        ::boost::make_transform_iterator(y.end(), poly_mul_impl_ptr_extractor{}));

    // Do the monomial overflow checking, if possible.
    const auto r1
        = ::obake::detail::make_range(::boost::make_transform_iterator(v1.cbegin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(v1.cend(), poly_term_key_ref_extractor{}));
    const auto r2
        = ::obake::detail::make_range(::boost::make_transform_iterator(v2.cbegin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(v2.cend(), poly_term_key_ref_extractor{}));
    if constexpr (are_overflow_testable_monomial_ranges_v<decltype(r1) &, decltype(r2) &>) {
        // Do the monomial overflow checking.
        if (obake_unlikely(!::obake::monomial_range_overflow_check(r1, r2, ss))) {
            obake_throw(
                ::std::overflow_error,
                "An overflow in the monomial exponents was detected while attempting to multiply two polynomials");
        }
    }

    // This functor will be used to compute the
    // upper limit of the j index (in the usual half-open
    // range fashion) for a given i index
    // in the nested for-loop below that performs
    // the term-by-term multiplications.
    // In the non-truncated case, compute_j_end will
    // always return the size of y (that is, every
    // term of x will be multiplied by every term of
    // of y). In the truncated cases, the returned
    // j value will ensure that the truncation limits
    // are respected.
    auto compute_j_end = [&v1, &v2, &ss, &args...]() {
        if constexpr (sizeof...(args) == 0u) {
            ::obake::detail::ignore(v1, ss);

            return [v2_size = v2.size()](const auto &) { return v2_size; };
        } else {
            // Helper that will:
            //
            // - create and return a sorted vector vd
            //   containing the total/partial degrees of all the
            //   terms in v,
            // - sort v according to the order defined in vd.
            //
            // v will be one of v1/v2, t is a type_c instance
            // containing either T or U.
            auto sorter = [&ss, &args...](auto &v, auto t) {
                // NOTE: we will be using the machinery from the default implementation
                // of degree() for series, so that we can re-use the concept checking bits
                // as well.
                using s_t = typename decltype(t)::type;

                // Compute the vector of degrees.
                auto vd = [&v, &ss, &args...]() {
                    if constexpr (sizeof...(args) == 1u) {
                        // Total degree.
                        ::obake::detail::ignore(args...);

                        // NOTE: in the make_degree_vector() helper we need
                        // to compute the size of v via iterator differences.
                        ::obake::detail::container_it_diff_check(v);

                        return customisation::internal::make_degree_vector<s_t>(v.cbegin(), v.cend(), ss, false);
                    } else {
                        // NOTE: in the make_p_degree_vector() helper we need
                        // to compute the size of v via iterator differences.
                        ::obake::detail::container_it_diff_check(v);

                        return customisation::internal::make_p_degree_vector<s_t>(
                            v.cbegin(), v.cend(), ss, ::std::get<1>(::std::forward_as_tuple(args...)), false);
                    }
                }();

                // Ensure that the size of vd is representable by the
                // diff type of its iterators. We'll need to do some
                // iterator arithmetics below.
                // NOTE: cast to const as we will use cbegin/cend below.
                ::obake::detail::container_it_diff_check(::std::as_const(vd));

                // Create a vector of indices into vd.
                ::std::vector<decltype(vd.size())> vidx;
                vidx.resize(::obake::safe_cast<decltype(vidx.size())>(vd.size()));
                ::std::iota(vidx.begin(), vidx.end(), decltype(vd.size())(0));

                // Sort indirectly.
                // NOTE: capture vd as const ref because in the lt-comparable requirements for the degree
                // type we are using const lrefs.
                ::std::sort(vidx.begin(), vidx.end(), [&vdc = ::std::as_const(vd)](const auto &idx1, const auto &idx2) {
                    return vdc[idx1] < vdc[idx2];
                });

                // Apply the sorting to vd and v. Check that permutated
                // access does not result in overflow.
                ::obake::detail::container_it_diff_check(vd);
                // NOTE: use cbegin/cend on vd to ensure the copy ctor of
                // the degree type is being called.
                vd = decltype(vd)(::boost::make_permutation_iterator(vd.cbegin(), vidx.cbegin()),
                                  ::boost::make_permutation_iterator(vd.cend(), vidx.cend()));
                ::obake::detail::container_it_diff_check(v);
                v = ::std::remove_reference_t<decltype(v)>(
                    ::boost::make_permutation_iterator(v.cbegin(), vidx.cbegin()),
                    ::boost::make_permutation_iterator(v.cend(), vidx.cend()));

#if !defined(NDEBUG)
                // Check the results in debug mode.
                // NOTE: use cbegin/cend in order to ensure
                // that the degrees are compared via const lvalue refs.
                assert(::std::is_sorted(vd.cbegin(), vd.cend()));

                if constexpr (sizeof...(args) == 1u) {
                    using d_impl = customisation::internal::series_default_degree_impl;

                    assert(::std::equal(vd.begin(), vd.end(),
                                        ::boost::make_transform_iterator(v.cbegin(), d_impl::d_extractor<s_t>{&ss}),
                                        [](const auto &a, const auto &b) { return !(a < b) && !(b < a); }));
                } else {
                    using d_impl = customisation::internal::series_default_p_degree_impl;

                    const auto &s = ::std::get<1>(::std::forward_as_tuple(args...));
                    const auto si = ::obake::detail::ss_intersect_idx(s, ss);

                    assert(::std::equal(
                        vd.begin(), vd.end(),
                        ::boost::make_transform_iterator(v.cbegin(), d_impl::d_extractor<s_t>{&s, &si, &ss}),
                        [](const auto &a, const auto &b) { return !(a < b) && !(b < a); }));
                }
#endif

                return vd;
            };

            using ::obake::detail::type_c;
            return [vd1 = sorter(v1, type_c<T>{}), vd2 = sorter(v2, type_c<U>{}),
                    // NOTE: max_deg is captured via const lref this way,
                    // as args is passed as a const lref pack.
                    &max_deg = ::std::get<0>(::std::forward_as_tuple(args...))](const auto &i) {
                using idx_t = remove_cvref_t<decltype(i)>;

                // Get the total/partial degree of the current term
                // in the first series.
                const auto &d_i = vd1[i];

                // Find the first term in the second series such
                // that d_i + d_j > max_deg.
                const auto it
                    = ::std::upper_bound(vd2.cbegin(), vd2.cend(), max_deg, [&d_i](const auto &mdeg, const auto &d_j) {
                          // NOTE: we require below
                          // comparability between const lvalue limit
                          // and rvalue of the sum of the degrees.
                          return mdeg < d_i + d_j;
                      });

                // Turn the iterator into an index and return it.
                // NOTE: we checked above that the iterator diff
                // type can safely be used as an index (for both
                // vd1 and vd2).
                return static_cast<idx_t>(it - vd2.cbegin());
            };
        }
    }();

    // Proceed with the multiplication.
    auto &tab = retval._get_s_table()[0];

    try {
        // Temporary variable used in monomial multiplication.
        ret_key_t tmp_key(ss);

        const auto v1_size = v1.size();
        for (decltype(v1.size()) i = 0; i < v1_size; ++i) {
            const auto &t1 = v1[i];
            const auto &k1 = t1->first;
            const auto &c1 = t1->second;

            // Get the upper limit of the multiplication
            // range in v2.
            const auto j_end = compute_j_end(i);
            if (sizeof...(Args) != 0u && j_end == 0u) {
                // In truncated mode, if j_end is zero, we don't need to perform
                // any more term multiplications as the remaining
                // ones will all end up above the truncation limit.
                break;
            }

            for (decltype(v2.size()) j = 0; j < j_end; ++j) {
                const auto &t2 = v2[j];
                const auto &c2 = t2->second;

                // Multiply the monomial.
                ::obake::monomial_mul(tmp_key, k1, t2->first, ss);

                // Try to insert the new term.
                // NOTE: see the explanation in the other
                // multiplication function about why we adopt
                // this scheme (i.e., default-emplace the coefficient).
                const auto res = tab.try_emplace(tmp_key);

                // NOTE: optimise with likely/unlikely here?
                if (res.second) {
                    res.first->second = c1 * c2;
                } else {
                    // The insertion failed, accumulate c1*c2 into the
                    // existing coefficient.
                    // NOTE: do it with fma3(), if possible.
                    if constexpr (is_mult_addable_v<ret_cf_t &, const cf1_t &, const cf2_t &>) {
                        ::obake::fma3(res.first->second, c1, c2);
                    } else {
                        res.first->second += c1 * c2;
                    }
                }
            }
        }

        // Determine and remove the keys whose coefficients are zero
        // in the return value.
        const auto it_f = tab.end();
        for (auto it = tab.begin(); it != it_f;) {
            // NOTE: abseil's flat_hash_map returns void on erase(),
            // thus we need to increase 'it' before possibly erasing.
            // erase() does not cause rehash and thus will not invalidate
            // any other iterator apart from the one being erased.
            if (obake_unlikely(::obake::is_zero(::std::as_const(it->second)))) {
                tab.erase(it++);
            } else {
                ++it;
            }
        }

        // NOTE: no need to check the table size, as retval
        // is not segmented.
        // LCOV_EXCL_START
    } catch (...) {
        // retval may now contain zero coefficients.
        // Make sure to clear it before rethrowing.
        tab.clear();
        throw;
        // LCOV_EXCL_STOP
    }
}

// Implementation of poly multiplication with identical symbol sets.
// Requires that x is not longer than y.
template <typename T, typename U, typename... Args>
inline auto poly_mul_impl_identical_ss(T &&x, U &&y, const Args &... args)
{
    using ret_t = poly_mul_ret_t<T &&, U &&>;
    using ret_key_t = series_key_t<ret_t>;

    // Check the preconditions.
    assert(x.size() <= y.size());
    assert(x.get_symbol_set() == y.get_symbol_set());

    // Init the return value.
    ret_t retval;
    retval.set_symbol_set(x.get_symbol_set());

    if (x.empty() || y.empty()) {
        // Exit early if either series is empty.
        return retval;
    }

    if constexpr (::std::conjunction_v<
                      is_homomorphically_hashable_monomial<ret_key_t>,
                      // Need also to be able to measure the byte size
                      // of x, y, and the key/cf of ret_t, via const lvalue references.
                      // NOTE: perhaps this is too much of a hard requirement,
                      // and we can make this optional (if not supported,
                      // fix the nsegs to something like twice the cores).
                      is_size_measurable<const remove_cvref_t<T> &>, is_size_measurable<const remove_cvref_t<U> &>,
                      is_size_measurable<const ret_key_t &>, is_size_measurable<const series_cf_t<ret_t> &>>) {
        // Homomorphic hashing is available, we can run
        // the multi-threaded implementation.

        // Establish the max byte size of the input series.
        const auto max_bs = ::std::max(::obake::byte_size(::std::as_const(x)), ::obake::byte_size(::std::as_const(y)));

        if ((x.size() == 1u && y.size() == 1u) || max_bs < 30000ul || ::obake::detail::hc() == 1u) {
            // Run the simple implementation if either:
            // - both polys have only 1 term, or
            // - the maximum operand size is less than a threshold value, or
            // - we have just 1 core.
            detail::poly_mul_impl_simple(retval, x, y, args...);
        } else {
            // Otherwise, run the MT implementation.
            detail::poly_mul_impl_mt_hm(retval, x, y, args...);
        }
    } else {
        // The monomial does not have homomorphic hashing,
        // just use the simple implementation.
        detail::poly_mul_impl_simple(retval, x, y, args...);
    }

    return retval;
}

// Top level function for poly multiplication. Requires that
// x is not longer than y.
// NOTE: future improvements:
// - make the ntrials for the estimation of the average term size
//   dependent on the number of term-by-term multiplications (need data for that).
// NOTE: performance considerations:
// - the multithreaded implementation still computes
//   the degrees of the terms of the input series twice. This
//   could be reduced at the price of changing a bit the code structure
//   and at the cost of additional indirect sorting (because we
//   would be computing the degree vectors at the beginning and then
//   we would need to sort them for segmentation purposes). It's not
//   clear to me if this is worth it at this time, need to profile;
// - in highly rectangular multiplications, quite a bit of time
//   is spent copying the larger operand into a vector of terms.
//   Perhaps this could be parallelised for segmented series?
// - in highly rectangular multiplications, the series size
//   estimation is quite poor (see comments on top of the
//   function). Not sure what we could do about it;
// - when constructing vectors of indices, or when building
//   vectors of degrees, we could improve performance by using
//   a default-constructing allocator, in order to avoid zeroing
//   out data which we will be overwriting anyway;
// - perhaps vector permutations could be done in parallel?
template <typename T, typename U, typename... Args>
inline auto poly_mul_impl(T &&x, U &&y, const Args &... args)
{
    // Check the precondition.
    assert(x.size() <= y.size());

    if (x.get_symbol_set() == y.get_symbol_set()) {
        return detail::poly_mul_impl_identical_ss(::std::forward<T>(x), ::std::forward<U>(y), args...);
    } else {
        using rT = remove_cvref_t<T>;
        using rU = remove_cvref_t<U>;

        // Merge the symbol sets.
        const auto &[merged_ss, ins_map_x, ins_map_y]
            = ::obake::detail::merge_symbol_sets(x.get_symbol_set(), y.get_symbol_set());

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
                ::obake::detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

                return detail::poly_mul_impl_identical_ss(::std::forward<T>(x), ::std::move(b), args...);
            }
            case 2u: {
                // y already has the correct symbol
                // set, extend only x.
                rT a;
                a.set_symbol_set(merged_ss);
                ::obake::detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);

                return detail::poly_mul_impl_identical_ss(::std::move(a), ::std::forward<U>(y), args...);
            }
        }

        // Both x and y need to be extended.
        rT a;
        rU b;
        a.set_symbol_set(merged_ss);
        b.set_symbol_set(merged_ss);
        ::obake::detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);
        ::obake::detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

        return detail::poly_mul_impl_identical_ss(::std::move(a), ::std::move(b), args...);
    }
}

// Helper to ensure that poly_mul_impl() is called with the
// shorter poly first, switching around the arguments if necessary.
template <typename T, typename U, typename... Args>
inline auto poly_mul_impl_switch(T &&x, U &&y, const Args &... args)
{
    if (x.size() <= y.size()) {
        return detail::poly_mul_impl(::std::forward<T>(x), ::std::forward<U>(y), args...);
    } else {
        return detail::poly_mul_impl(::std::forward<U>(y), ::std::forward<T>(x), args...);
    }
}

} // namespace detail

template <typename T, typename U, ::std::enable_if_t<detail::poly_mul_algo<T &&, U &&> != 0, int> = 0>
inline detail::poly_mul_ret_t<T &&, U &&> series_mul(T &&x, U &&y)
{
    return detail::poly_mul_impl_switch(::std::forward<T>(x), ::std::forward<U>(y));
}

namespace detail
{

// Metaprogramming to establish if we can perform
// truncated total/partial degree multiplication on the
// polynomial operands T and U with degree limit of type V.
// NOTE: at this time, truncated multiplication is implemented
// only if only the key is with degree.
template <typename T, typename U, typename V, bool Total>
constexpr auto poly_mul_truncated_degree_algorithm_impl()
{
    // Check first if we can do the untruncated multiplication. If we cannot,
    // truncated mult is not possible.
    if constexpr (poly_mul_algo<T, U> == 0) {
        return 0;
    } else {
        // Check if we can compute the degree of the terms via the default
        // implementation for series.
        using d_impl = ::std::conditional_t<Total, customisation::internal::series_default_degree_impl,
                                            customisation::internal::series_default_p_degree_impl>;
        constexpr auto algo1 = d_impl::template algo<T>;
        constexpr auto algo2 = d_impl::template algo<U>;

        // NOTE: algo == 3 means that only the key is with degree.
        if constexpr (algo1 == 3 && algo2 == 3) {
            // Fetch the degree types.
            using deg1_t = typename d_impl::template ret_t<T>;
            using deg2_t = typename d_impl::template ret_t<U>;

            // The degree types need to be:
            // - copy/move ctible (need to handle vectors of them, note that
            //   in the series default degree machinery we check for returnability,
            //   but here we need explicitly both copy and move ctible),
            // - addable via const lref,
            // - V must be lt-comparable to the type of their sum via const lrefs.
            using deg_add_t = detected_t<::obake::detail::add_t, const deg1_t &, const deg2_t &>;

            return static_cast<int>(::std::conjunction_v<
                                    // NOTE: this will ensure that switching around
                                    // the degree operands still works and produces the same
                                    // type. This is necessary because in the poly moltiplication
                                    // function we might end up swapping the operands.
                                    // NOTE: this will also verify that deg_add_t is present.
                                    is_addable<const deg1_t &, const deg2_t &>, ::std::is_copy_constructible<deg1_t>,
                                    ::std::is_copy_constructible<deg2_t>, ::std::is_move_constructible<deg1_t>,
                                    ::std::is_move_constructible<deg2_t>,
                                    // NOTE: in the implementation functions, we always compare
                                    // an lvalue ref to the limit V to an rvalue resulting
                                    // from adding the degrees of one term from each series.
                                    is_less_than_comparable<::std::add_lvalue_reference_t<const V>, deg_add_t>>);
        } else {
            return 0;
        }
    }
}

template <typename T, typename U, typename V>
inline constexpr auto poly_mul_truncated_degree_algo
    = detail::poly_mul_truncated_degree_algorithm_impl<T, U, V, true>();

template <typename T, typename U, typename V>
inline constexpr auto poly_mul_truncated_p_degree_algo
    = detail::poly_mul_truncated_degree_algorithm_impl<T, U, V, false>();

} // namespace detail

// Truncated multiplication.
// NOTE: do we need the type traits/concepts as well?
// NOTE: should these be function objects?
template <typename T, typename U, typename V,
          ::std::enable_if_t<detail::poly_mul_truncated_degree_algo<T &&, U &&, V> != 0, int> = 0>
inline detail::poly_mul_ret_t<T &&, U &&> truncated_mul(T &&x, U &&y, const V &max_degree)
{
    return detail::poly_mul_impl_switch(::std::forward<T>(x), ::std::forward<U>(y), max_degree);
}

template <typename T, typename U, typename V,
          ::std::enable_if_t<detail::poly_mul_truncated_p_degree_algo<T &&, U &&, V> != 0, int> = 0>
inline detail::poly_mul_ret_t<T &&, U &&> truncated_mul(T &&x, U &&y, const V &max_degree, const symbol_set &s)
{
    return detail::poly_mul_impl_switch(::std::forward<T>(x), ::std::forward<U>(y), max_degree, s);
}

namespace detail
{

// Machinery to enable the pow() specialisation for polynomials.
template <typename T, typename U>
constexpr auto poly_pow_algorithm_impl()
{
    if constexpr (is_polynomial_v<remove_cvref_t<T>>) {
        return customisation::internal::series_default_pow_impl::algo<T, U>;
    } else {
        return 0;
    }
}

template <typename T, typename U>
inline constexpr auto poly_pow_algo = detail::poly_pow_algorithm_impl<T, U>();

} // namespace detail

template <typename T, typename U, ::std::enable_if_t<detail::poly_pow_algo<T &&, U &&> != 0, int> = 0>
inline customisation::internal::series_default_pow_impl::ret_t<T &&, U &&> pow(T &&x, U &&y)
{
    using impl = customisation::internal::series_default_pow_impl;

    using rT = remove_cvref_t<T>;
    using rU = remove_cvref_t<U>;
    using key_t = series_key_t<rT>;

    if constexpr (is_exponentiable_monomial_v<const key_t &, const rU &>) {
        if (x.size() == 1u) {
            // The polynomial has a single term, we can proceed
            // with the monomial exponentiation.

            // Cache the symbol set.
            const auto &ss = x.get_symbol_set();

            // Build the return value.
            impl::ret_t<T &&, U &&> retval;
            retval.set_symbol_set(ss);
            // NOTE: we do coefficient and monomial exponentiation using
            // const refs everywhere (thus, we ensure that y is not mutated
            // after one exponentiation). Regarding coefficient exponentiation,
            // we checked in the default implementation that it is supported
            // via const lvalue refs.
            const auto it = x.cbegin();
            retval.add_term(::obake::monomial_pow(it->first, ::std::as_const(y), ss),
                            ::obake::pow(it->second, ::std::as_const(y)));

            return retval;
        } else {
            // The polynomial is empty or it has more than 1 term, perfect forward to
            // the series implementation.
            return impl{}(::std::forward<T>(x), ::std::forward<U>(y));
        }
    } else {
        // Cannot do monomial exponentiation, perfect forward to
        // the series implementation.
        return impl{}(::std::forward<T>(x), ::std::forward<U>(y));
    }
}

namespace detail
{

// Meta-programming for the selection of the
// polynomial substitution algorithm.
// NOTE: currently this supports only the case
// in which both cf and key are substitutable.
template <typename T, typename U>
constexpr auto poly_subs_algorithm_impl()
{
    using rT = remove_cvref_t<T>;

    // Shortcut for signalling that the subs implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, ::obake::detail::type_c<void>{});

    if constexpr (!is_polynomial_v<rT>) {
        // Not a polynomial.
        return failure;
    } else {
        using cf_t = series_cf_t<rT>;
        using key_t = series_key_t<rT>;

        if constexpr (::std::conjunction_v<is_substitutable_monomial<const key_t &, U>,
                                           is_substitutable<const cf_t &, U>>) {
            // Both cf and key support substitution (via const lvalue refs).
            using key_subs_t = typename ::obake::detail::monomial_subs_t<const key_t &, U>::first_type;
            using cf_subs_t = ::obake::detail::subs_t<const cf_t &, U>;

            // The type of the product of key_subs_t * cf_subs_t
            // (via rvalues).
            using subs_prod_t = detected_t<::obake::detail::mul_t, key_subs_t, cf_subs_t>;
            // The candidate return type: rvalue subs_prod_t * const lvalue ref rT.
            using ret_t = detected_t<::obake::detail::mul_t, subs_prod_t, const rT &>;

            // ret_t must be addable in place with an rvalue, and the original coefficient
            // type must be constructible from int.
            if constexpr (::std::conjunction_v<
                              // NOTE: verify the detection of subs_prod_t, as it is used
                              // in ret_t.
                              is_detected<::obake::detail::mul_t, key_subs_t, cf_subs_t>,
                              is_in_place_addable<::std::add_lvalue_reference_t<ret_t>, ret_t>,
                              ::std::is_constructible<cf_t, int>>) {
                return ::std::make_pair(1, ::obake::detail::type_c<ret_t>{});
            } else {
                return failure;
            }
        } else {
            return failure;
        }
    }
}

template <typename T, typename U>
inline constexpr auto poly_subs_algorithm = detail::poly_subs_algorithm_impl<T, U>();

template <typename T, typename U>
inline constexpr int poly_subs_algo = poly_subs_algorithm<T, U>.first;

template <typename T, typename U>
using poly_subs_ret_t = typename decltype(poly_subs_algorithm<T, U>.second)::type;

} // namespace detail

template <typename T, typename U, ::std::enable_if_t<detail::poly_subs_algo<T &&, U> != 0, int> = 0>
inline detail::poly_subs_ret_t<T &&, U> subs(T &&x_, const symbol_map<U> &sm)
{
    // Sanity check.
    static_assert(detail::poly_subs_algo<T &&, U> == 1);

    // Need only const access to x.
    const auto &x = ::std::as_const(x_);

    // Cache a reference to the symbol set.
    const auto &ss = x.get_symbol_set();

    // Compute the intersection between sm and ss.
    const auto si = ::obake::detail::sm_intersect_idx(sm, ss);

    // Init a temp poly that we will use in the loop below.
    remove_cvref_t<T> tmp_poly;
    tmp_poly.set_symbol_set(ss);

    // The return value (this will default-construct
    // an empty polynomial).
    detail::poly_subs_ret_t<T &&, U> retval;

    // NOTE: parallelisation opportunities here
    // for segmented tables.
    for (const auto &t : x) {
        const auto &k = t.first;
        const auto &c = t.second;

        // Do the monomial substitution.
        auto k_sub(::obake::monomial_subs(k, si, ss));

        // Clear up tmp_poly, add a term with unitary
        // coefficient containing the monomial result of the
        // substitution above.
        tmp_poly.clear_terms();
        tmp_poly.add_term(::std::move(k_sub.second), 1);

        // Compute the product of the substitutions and accumulate
        // it into the return value.
        // NOTE: if the type of retval coincides with the type
        // of the original poly, we could probably optimise this
        // to do term insertions rather than going through with
        // the multiplications. E.g., substitution with integral
        // values in a polynomial with integral coefficients.
        // NOTE: another optimisation is possible if
        // the type of subs(c, sm) is the coefficient type of
        // tmp_poly. In that case we can avoid one multiplication
        // and call tmp_poly.add_term() above directly with
        // subs(c, sm) as a coefficient instead of 1.
        retval += ::std::move(k_sub.first) * ::obake::subs(c, sm) * ::std::as_const(tmp_poly);
    }

    return retval;
}

namespace detail
{

// Meta-programming for the selection of the
// truncate_degree() algorithm.
// NOTE: at this time, we support only truncation
// based on key filtering.
template <typename T, typename U>
constexpr int poly_truncate_degree_algorithm_impl()
{
    if constexpr (!is_polynomial_v<::std::remove_reference_t<T>>) {
        // Not a mutable polynomial.
        return 0;
    } else {
        // Check if we can compute the degree of the terms via the default
        // implementation for series.
        using d_impl = customisation::internal::series_default_degree_impl;
        constexpr auto algo = d_impl::template algo<T>;

        if constexpr (algo == 3) {
            // The truncation will involve only key-based
            // filtering. We need to be able to lt-compare U
            // to the degree type of the key (const lvalue
            // ref vs rvalue).
            using deg_t = typename d_impl::template ret_t<T>;

            return is_less_than_comparable_v<::std::add_lvalue_reference_t<const remove_cvref_t<U>>, deg_t> ? 1 : 0;
        } else {
            // The key degree computation is not possible, or it
            // involves the coefficient in addition to the key.
            // NOTE: the case in which the coefficient is degree
            // truncatable and the key does not support degree computation
            // may eventually be handled in a default series implementation
            // of truncate_degree().
            return 0;
        }
    }
}

template <typename T, typename U>
inline constexpr int poly_truncate_degree_algo = detail::poly_truncate_degree_algorithm_impl<T, U>();

} // namespace detail

// NOTE: perhaps we can extend the implementation in the future
// to accept mutable rvalue references as well.
template <typename T, typename U, ::std::enable_if_t<detail::poly_truncate_degree_algo<T &, U &&> != 0, int> = 0>
inline void truncate_degree(T &x, U &&y_)
{
    // Sanity checks.
    static_assert(detail::poly_truncate_degree_algo<T &, U &&> == 1);

    // Use the default functor for the extraction of the term degree.
    using d_impl = customisation::internal::series_default_degree_impl;

    // Implement on top of filter().
    // NOTE: d_extractor will strip out the cvref
    // from T, thus we can just pass in T as-is.
    ::obake::filter(x, [deg_ext = d_impl::d_extractor<T>{&x.get_symbol_set()},
                        &y = ::std::as_const(y_)](const auto &t) { return !(y < deg_ext(t)); });
}

namespace detail
{

// Meta-programming for the selection of the
// truncate_p_degree() algorithm.
// NOTE: at this time, we support only truncation
// based on key filtering.
template <typename T, typename U>
constexpr int poly_truncate_p_degree_algorithm_impl()
{
    if constexpr (!is_polynomial_v<::std::remove_reference_t<T>>) {
        // Not a mutable polynomial.
        return 0;
    } else {
        // Check if we can compute the p_degree of the terms via the default
        // implementation for series.
        using d_impl = customisation::internal::series_default_p_degree_impl;
        constexpr auto algo = d_impl::template algo<T>;

        if constexpr (algo == 3) {
            // The truncation will involve only key-based
            // filtering. We need to be able to lt-compare U
            // to the partial degree type of the key (const lvalue
            // ref vs rvalue).
            using deg_t = typename d_impl::template ret_t<T>;

            return is_less_than_comparable_v<::std::add_lvalue_reference_t<const remove_cvref_t<U>>, deg_t> ? 1 : 0;
        } else {
            // The key partial degree computation is not possible, or it
            // involves the coefficient in addition to the key.
            // NOTE: the case in which the coefficient is partial degree
            // truncatable and the key does not support partial degree computation
            // may eventually be handled in a default series implementation
            // of truncate_p_degree().
            return 0;
        }
    }
}

template <typename T, typename U>
inline constexpr int poly_truncate_p_degree_algo = detail::poly_truncate_p_degree_algorithm_impl<T, U>();

} // namespace detail

// NOTE: perhaps we can extend the implementation in the future
// to accept mutable rvalue references as well.
template <typename T, typename U, ::std::enable_if_t<detail::poly_truncate_p_degree_algo<T &, U &&> != 0, int> = 0>
inline void truncate_p_degree(T &x, U &&y_, const symbol_set &s)
{
    // Sanity checks.
    static_assert(detail::poly_truncate_p_degree_algo<T &, U &&> == 1);

    // Use the default functor for the extraction of the term partial degree.
    using d_impl = customisation::internal::series_default_p_degree_impl;

    // Extract the symbol indices.
    const auto &ss = x.get_symbol_set();
    const auto si = ::obake::detail::ss_intersect_idx(s, ss);

    // Implement on top of filter().
    // NOTE: d_extractor will strip out the cvref
    // from T, thus we can just pass in T as-is.
    ::obake::filter(x, [deg_ext = d_impl::d_extractor<T>{&s, &si, &ss}, &y = ::std::as_const(y_)](const auto &t) {
        return !(y < deg_ext(t));
    });
}

namespace detail
{

// Meta-programming for the selection of the
// diff() algorithm.
template <typename T>
constexpr auto poly_diff_algorithm_impl()
{
    using rT = remove_cvref_t<T>;

    // Shortcut for signalling that the diff() implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, ::obake::detail::type_c<void>{});

    if constexpr (!is_polynomial_v<rT>) {
        // Not a polynomial.
        return failure;
    } else {
        using cf_t = series_cf_t<rT>;
        using key_t = series_key_t<rT>;

        if constexpr (::std::conjunction_v<is_differentiable<const cf_t &>,
                                           is_differentiable_monomial<const key_t &>>) {
            // Both cf and key are differentiable.
            using cf_diff_t = ::obake::detail::diff_t<const cf_t &>;
            using key_diff_t = typename ::obake::detail::monomial_diff_t<const key_t &>::first_type;

            // Fetch the type of the multiplication of the coefficient
            // by key_diff_t (const lref vs rvalue).
            using prod1_t = detected_t<::obake::detail::mul_t, const cf_t &, key_diff_t>;

            if constexpr (::std::conjunction_v<::std::is_same<cf_diff_t, cf_t>,
                                               // NOTE: this condition also checks
                                               // that prod1_t is detected (nonesuch
                                               // cannot be a coefficient type).
                                               ::std::is_same<prod1_t, cf_t>>) {
                // Special case: the differentiation of the coefficient does not change its type,
                // and prod1_t is the original coefficient type. In this case, we can implement
                // differentiation by repeated term insertions on a series of type rT.
                return ::std::make_pair(2, ::obake::detail::type_c<rT>{});
            } else {
                // General case: deduce the return type via the
                // product rule.
                using prod2_t = detected_t<::obake::detail::mul_t, cf_diff_t, const rT &>;
                using prod3_t = detected_t<::obake::detail::mul_t, prod1_t, const rT &>;
                // The candidate return type.
                using s_t = detected_t<::obake::detail::add_t, prod2_t, prod3_t>;

                if constexpr (::std::conjunction_v<
                                  // Verify the detection of the type aliases above.
                                  is_detected<::obake::detail::mul_t, const cf_t &, key_diff_t>,
                                  is_detected<::obake::detail::mul_t, cf_diff_t, const rT &>,
                                  is_detected<::obake::detail::mul_t, prod1_t, const rT &>,
                                  // The return type must be accumulable.
                                  // NOTE: this condition also checks that s_t is detected,
                                  // as nonesuch is not in-place addable.
                                  is_in_place_addable<::std::add_lvalue_reference_t<s_t>, s_t>,
                                  // Need also to add in place with prod2_t in case
                                  // the differentiation variable is not in the polynomial.
                                  is_in_place_addable<::std::add_lvalue_reference_t<s_t>, prod2_t>,
                                  // Need to init s_t to zero for accumulation.
                                  ::std::is_constructible<s_t, int>,
                                  // Need to construct cf_t from 1 for the temporary
                                  // polynomials.
                                  ::std::is_constructible<cf_t, int>>) {
                    return ::std::make_pair(1, ::obake::detail::type_c<s_t>{});
                } else {
                    return failure;
                }
            }
        } else {
            return failure;
        }
    }
}

template <typename T>
inline constexpr auto poly_diff_algorithm = detail::poly_diff_algorithm_impl<T>();

template <typename T>
inline constexpr int poly_diff_algo = poly_diff_algorithm<T>.first;

template <typename T>
using poly_diff_ret_t = typename decltype(poly_diff_algorithm<T>.second)::type;

} // namespace detail

template <typename T, ::std::enable_if_t<detail::poly_diff_algo<T &&> != 0, int> = 0>
inline detail::poly_diff_ret_t<T &&> diff(T &&x_, const ::std::string &s)
{
    using ret_t = detail::poly_diff_ret_t<T &&>;
    constexpr auto algo = detail::poly_diff_algo<T &&>;

    // Sanity checks.
    static_assert(algo == 1 || algo == 2);

    // Need only const access to x.
    const auto &x = ::std::as_const(x_);

    // Cache the symbol set.
    const auto &ss = x.get_symbol_set();

    // Determine the index of s in the symbol set.
    const auto idx = ss.index_of(ss.find(s));
    // The symbol is present if its index
    // is not ss.size().
    const auto s_present = (idx != ss.size());

    if constexpr (algo == 1) {
        // The general algorithm.
        // Init temp polys that we will use in the loop below.
        // These will represent the original monomial and its
        // derivative as series of type T (after cvref removal).
        remove_cvref_t<T> tmp_p1, tmp_p2;
        tmp_p1.set_symbol_set(ss);
        tmp_p2.set_symbol_set(ss);

        ret_t retval(0);
        for (const auto &t : x) {
            const auto &k = t.first;
            const auto &c = t.second;

            // Prepare the first temp poly.
            tmp_p1.clear_terms();
            tmp_p1.add_term(k, 1);

            if (s_present) {
                // The symbol is present in the symbol set,
                // need to diff the monomial.
                auto key_diff(::obake::monomial_diff(k, idx, ss));

                // Prepare the second temp poly.
                tmp_p2.clear_terms();
                tmp_p2.add_term(::std::move(key_diff.second), 1);

                // Put everything together.
                retval += ::obake::diff(c, s) * ::std::as_const(tmp_p1)
                          + c * ::std::move(key_diff.first) * ::std::as_const(tmp_p2);
            } else {
                // The symbol is not present in the symbol set,
                // need to diff only the coefficient.
                retval += ::obake::diff(c, s) * ::std::as_const(tmp_p1);
            }
        }

        return retval;
    } else {
        // Faster implementation via term insertions.
        // The return type must be the original poly type.
        static_assert(::std::is_same_v<ret_t, remove_cvref_t<T>>);

        // Init retval, using the same symbol set
        // and segmentation from x, and reserving
        // the same size as x.
        ret_t retval;
        retval.set_symbol_set(ss);
        retval.set_n_segments(x.get_s_size());
        retval.reserve(x.size());

        for (const auto &t : x) {
            const auto &k = t.first;
            const auto &c = t.second;

            // Add the term corresponding to the differentiation
            // of the coefficient.
            // NOTE: generally speaking, here we probably need
            // all insertion checks:
            // - mixing diffed and non-diffed
            //   monomials in retval will produce non-unique
            //   monomials,
            // - diff on coefficients/keys may result in zero,
            // - table size could end up being anything.
            // Probably the only check we can currently drop is about
            // monomial compatibility. Keep it in mind for the future,
            // if performance becomes a concern. Also, as usual,
            // we could differentiate between segmented and non
            // segmented layouts to improve performance.
            retval.add_term(k, ::obake::diff(c, s));

            if (s_present) {
                // The symbol is present in the symbol set,
                // need to diff the monomial too.
                auto key_diff(::obake::monomial_diff(k, idx, ss));
                retval.add_term(::std::move(key_diff.second), c * ::std::move(key_diff.first));
            }
        }

        return retval;
    }
}

namespace detail
{

// Meta-programming for the selection of the
// integrate() algorithm.
// NOTE: this currently supports only the case
// in which the monomial is integrable and the
// coefficient is differentiable, with a derivative
// which is always zero (i.e., constant coefficients).
template <typename T>
constexpr auto poly_integrate_algorithm_impl()
{
    using rT = remove_cvref_t<T>;

    // Shortcut for signalling that the integrate() implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, ::obake::detail::type_c<void>{});

    if constexpr (!is_polynomial_v<rT>) {
        // Not a polynomial.
        return failure;
    } else {
        using cf_t = series_cf_t<rT>;
        using key_t = series_key_t<rT>;

        if constexpr (::std::conjunction_v<is_differentiable<const cf_t &>, is_integrable_monomial<const key_t &>,
                                           is_symbols_mergeable_key<const key_t &>>) {
            // cf is differentiable, key is integrable and symbol mergeable.
            using cf_diff_t = ::obake::detail::diff_t<const cf_t &>;
            using key_int_t = typename ::obake::detail::monomial_integrate_t<const key_t &>::first_type;

            // Fetch the type of the division of the coefficient
            // by key_int_t (const lref vs rvalue).
            using quot_t = detected_t<::obake::detail::div_t, const cf_t &, key_int_t>;

            if constexpr (::std::conjunction_v<is_zero_testable<cf_diff_t>,
                                               // NOTE: this condition also checks
                                               // that quot_t is detected (nonesuch
                                               // cannot be a coefficient type).
                                               ::std::is_same<quot_t, cf_t>>) {
                // Special case: the differentiation of the coefficient is zero-testable,
                // and quot_t is the original coefficient type. In this case, we can implement
                // integration by repeated term insertions on a series of type rT.
                return ::std::make_pair(2, ::obake::detail::type_c<rT>{});
            } else {
                // General case: the return type is quot_t * const rT &.
                using ret_t = detected_t<::obake::detail::mul_t, quot_t, const rT &>;

                if constexpr (::std::conjunction_v<
                                  // Ensure quot_t is detected, as we use it in the definition of ret_t.
                                  is_detected<::obake::detail::div_t, const cf_t &, key_int_t>,
                                  // The return type must be accumulable.
                                  // NOTE: this condition also checks that ret_t is detected,
                                  // as nonesuch is not in-place addable.
                                  is_in_place_addable<::std::add_lvalue_reference_t<ret_t>, ret_t>,
                                  // Need to init ret_t to zero for accumulation.
                                  ::std::is_constructible<ret_t, int>,
                                  // Need to construct cf_t from 1 for the temporary
                                  // polynomials.
                                  ::std::is_constructible<cf_t, int>,
                                  // Need to test that the derivative of the coefficient is zero.
                                  is_zero_testable<cf_diff_t>>) {
                    return ::std::make_pair(1, ::obake::detail::type_c<ret_t>{});
                } else {
                    return failure;
                }
            }
        } else {
            return failure;
        }
    }
}

template <typename T>
inline constexpr auto poly_integrate_algorithm = detail::poly_integrate_algorithm_impl<T>();

template <typename T>
inline constexpr int poly_integrate_algo = poly_integrate_algorithm<T>.first;

template <typename T>
using poly_integrate_ret_t = typename decltype(poly_integrate_algorithm<T>.second)::type;

} // namespace detail

template <typename T, ::std::enable_if_t<detail::poly_integrate_algo<T &&> != 0, int> = 0>
inline detail::poly_integrate_ret_t<T &&> integrate(T &&x_, const ::std::string &s)
{
    using ret_t = detail::poly_integrate_ret_t<T &&>;
    using rT = remove_cvref_t<T>;

    // The implementation function.
    auto impl = [&s](const rT &x, symbol_idx idx) {
        constexpr auto algo = detail::poly_integrate_algo<T &&>;

        // Sanity check.
        static_assert(algo == 1 || algo == 2);

        // Cache the symbol set.
        const auto &ss = x.get_symbol_set();
        // idx has to be present.
        assert(idx != ss.size());

        // Error message if a coefficient has nonzero derivative with respect to s.
        const auto cf_diff_err_msg = "The current polynomial integration algorithm requires the derivatives of all "
                                     "coefficients with respect to the symbol '"
                                     + s + "' to be zero, but a coefficient with nonzero derivative was detected";

        if constexpr (algo == 1) {
            // The general algorithm.
            // Init temp poly that we will use in the loop below.
            // This will represent the integral of the original monomial
            // as a series of type rT.
            rT tmp_p;
            tmp_p.set_symbol_set(ss);

            // Produce retval by accumulation.
            ret_t retval(0);
            for (const auto &t : x) {
                const auto &c = t.second;

                if (obake_unlikely(!::obake::is_zero(::obake::diff(c, s)))) {
                    obake_throw(::std::invalid_argument, cf_diff_err_msg);
                }

                // Do the monomial integration.
                auto key_int(::obake::monomial_integrate(t.first, idx, ss));

                // Prepare the temp poly.
                tmp_p.clear_terms();
                tmp_p.add_term(::std::move(key_int.second), 1);

                // Put everything together.
                retval += c / ::std::move(key_int.first) * ::std::as_const(tmp_p);
            }

            return retval;
        } else {
            // Faster implementation via term insertions.
            // The return type must be the original poly type.
            static_assert(::std::is_same_v<ret_t, rT>);

            // Init retval, using the same symbol set
            // and segmentation from x, and reserving
            // the same size as x.
            ret_t retval;
            retval.set_symbol_set(ss);
            retval.set_n_segments(x.get_s_size());
            retval.reserve(x.size());

            for (const auto &t : x) {
                const auto &c = t.second;

                if (obake_unlikely(!::obake::is_zero(::obake::diff(c, s)))) {
                    obake_throw(::std::invalid_argument, cf_diff_err_msg);
                }

                // Do the monomial integration, mix it with the coefficient.
                auto key_int(::obake::monomial_integrate(t.first, idx, ss));
                // NOTE: here we could probably avoid most checks in the
                // term addition logic. Keep it in mind for the future.
                retval.add_term(::std::move(key_int.second), c / ::std::move(key_int.first));
            }

            return retval;
        }
    };

    // Cache the original symbol set.
    const auto &ss = x_.get_symbol_set();

    // Check if s is in the original symbol set.
    const auto it_s = ss.find(s);
    // Determine its index as well.
    const auto s_idx = ss.index_of(it_s);
    if (it_s == ss.cend() || *it_s != s) {
        // s is not in the original symbol set,
        // we will have to extend x with s.

        // Prepare the new symbol set.
        symbol_set::sequence_type seq;
        seq.reserve(ss.size() + 1u);
        seq.insert(seq.end(), ss.cbegin(), it_s);
        seq.push_back(s);
        seq.insert(seq.end(), it_s, ss.cend());
        assert(::std::is_sorted(seq.cbegin(), seq.cend()));
        symbol_set new_ss;
        new_ss.adopt_sequence(::boost::container::ordered_unique_range_t{}, ::std::move(seq));

        // Prepare the merged version of x.
        rT merged_x;
        merged_x.set_symbol_set(new_ss);
        ::obake::detail::series_sym_extender(merged_x, ::std::forward<T>(x_), symbol_idx_map<symbol_set>{{s_idx, {s}}});

        // Run the implementation on merged_x.
        return impl(merged_x, s_idx);
    } else {
        // s is in the original symbol set, run
        // the implementation directly on x_.
        return impl(x_, s_idx);
    }
}

} // namespace polynomials

} // namespace obake

#endif
