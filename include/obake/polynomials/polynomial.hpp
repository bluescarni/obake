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
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/container/container_fwd.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>

#include <mp++/integer.hpp>

#include <obake/byte_size.hpp>
#include <obake/config.hpp>
#include <obake/detail/abseil.hpp>
#include <obake/detail/container_it_diff_check.hpp>
#include <obake/detail/hc.hpp>
#include <obake/detail/ignore.hpp>
#include <obake/detail/ss_func_forward.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/type_c.hpp>
#include <obake/detail/xoroshiro128_plus.hpp>
#include <obake/exceptions.hpp>
#include <obake/hash.hpp>
#include <obake/key/key_degree.hpp>
#include <obake/key/key_merge_symbols.hpp>
#include <obake/math/degree.hpp>
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
using make_polynomials_enabler
    = ::std::enable_if_t<::std::conjunction_v<is_polynomial<T>, ::std::is_constructible<::std::string, const Args &>...,
                                              ::std::is_constructible<series_key_t<T>, const int *, const int *>,
                                              ::std::is_constructible<series_cf_t<T>, int>>,
                         int>;

// Overload with a symbol set.
template <typename T, typename... Args, make_polynomials_enabler<T, Args...> = 0>
inline ::std::array<T, sizeof...(Args)> make_polynomials_impl(const symbol_set &ss, const Args &... names)
{
    [[maybe_unused]] auto make_poly = [&ss](const auto &n) {
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
        ::std::vector<int> tmp(::obake::safe_cast<::std::vector<int>::size_type>(ss.size()));
        const auto it = ss.find(s);
        if (obake_unlikely(it == ss.end() || *it != s)) {
            obake_throw(::std::invalid_argument, "Cannot create a polynomial with symbol set " + detail::to_string(ss)
                                                     + " from the generator '" + s
                                                     + "': the generator is not in the symbol set");
        }

        // Set to 1 the exponent of the corresponding generator.
        tmp[static_cast<::std::vector<int>::size_type>(ss.index_of(it))] = 1;

        // Create and add a new term.
        retval.add_term(series_key_t<T>(::std::as_const(tmp).data(), ::std::as_const(tmp).data() + tmp.size()), 1);

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

#if defined(_MSC_VER)

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
                              // If ret_cf_t a coefficient type?
                              // NOTE: this checks ensures that ret_cf_t is detected,
                              // because nonesuch is not a coefficient type.
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

// Helper to estimate an appropriate log2 of the number of segments
// of the destination polynomial in a multithreaded homomorphic
// polynomial multiplication.
template <typename RetCf, typename T1, typename T2>
inline unsigned poly_mul_impl_mt_hm_compute_log2_nsegs(const ::std::vector<T1> &v1, const ::std::vector<T2> &v2,
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
    ::obake::detail::xoroshiro128_plus rng{s1 + static_cast<::std::uint64_t>(v1.size()),
                                           s2 + static_cast<::std::uint64_t>(v2.size())};

    // The idea now is to compute a small amount of term-by-term
    // multiplications and determine the average size in bytes
    // of the produced terms. From there, we'll try to estimate the
    // size in bytes of the series product and finally infer an
    // adequately small number of segments.
    constexpr int ntrials = 10;

    // Temporary monomial used for term-by-term multiplications.
    ret_key_t tmp_key;

    // Cache the series sizes.
    const auto v1_size = v1.size();
    const auto v2_size = v2.size();

    // Run the trials.
    ::std::size_t acc = 0;
    for (auto i = 0; i < ntrials; ++i) {
        // Pick a random term in each series.
        const auto idx1 = rng.template random<decltype(v1.size())>() % v1_size;
        const auto idx2 = rng.template random<decltype(v2.size())>() % v2_size;

        // Multiply monomial and coefficient.
        ::obake::monomial_mul(tmp_key, v1[idx1].first, v2[idx2].first, ss);
        const auto tmp_cf = v1[idx1].second * v2[idx2].second;

        // Accumulate the size of the produced term: size of monomial,
        // coefficient, and, if present, padding.
        acc += ::obake::byte_size(::std::as_const(tmp_key)) + ::obake::byte_size(tmp_cf) + pad_size;
    }
    // Compute the average term size.
    const auto avg_size = static_cast<double>(acc) / ntrials;

    // Estimate the total byte size of the series product. The heuristic
    // is based on a very sparse case (i.e., we take a small percentage
    // of the size of the product of a completely sparse multitplication). If the multiplication
    // is denser, we overestimate the total size and we have a higher
    // number of segments than necessary. Luckily, this does not
    // seem to hurt performance much.
    // NOTE: this factor might become a user-tunable parameter.
    // Need to test more.
    const auto est_total_size = .01 / 100. * (avg_size * static_cast<double>(v1_size) * static_cast<double>(v2_size));

    // Compute the number of segments by enforcing a fixed
    // amount of bytes per segment.
    const auto nsegs
        = ::boost::numeric_cast<typename polynomial<ret_key_t, RetCf>::s_size_type>(est_total_size / (500. * 1024.));

    // Finally, compute the log2 + 1 of nsegs and return it, but
    // make sure that it is not greater than the max_log2_size()
    // allowed in a series.
    // NOTE: even if nsegs is zero, this will still yield some
    // useful value, as nbits() on zero will return zero.
    return ::std::min(::obake::safe_cast<unsigned>(::mppp::integer<1>{nsegs}.nbits()),
                      polynomial<ret_key_t, RetCf>::get_max_s_size());
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

// Estimate the size of the product of two input polynomials.
// S1 and S2 are the types of the polyomials, x and y the polynomials
// represented as vectors of terms. The extra arguments represent
// the truncation limits.
// Requires x and y not empty, x not shorter than y. The returned
// value is guaranteed to be nonzero.
template <typename S1, typename S2, typename T, typename U, typename... Args>
inline auto poly_mul_estimate_product_size(const T &x, const U &y, const symbol_set &ss, const Args &... args)
{
    // Preconditions.
    assert(!x.empty());
    assert(!y.empty());
    assert(x.size() >= y.size());
    static_assert(sizeof...(args) <= 2u);

    // Make sure that the key types of T and U coincide.
    using key_type = typename T::value_type::first_type;
    static_assert(::std::is_same_v<key_type, typename U::value_type::first_type>);

    const auto size1 = x.size();
    const auto size2 = y.size();

    // If either series has a size of 1, just return size1 * size2.
    if (size1 == 1u || size2 == 1u) {
        return ::mppp::integer<1>{size1} * size2;
    }

    // Create the degree data. In untruncated multiplication,
    // this will just be an empty tuple, otherwise it will
    // be a pair containing the (partial) degree of the terms
    // of x and y in the original order.
    auto degree_data = [&x, &y, &ss, &args...]() {
        if constexpr (sizeof...(args) == 0u) {
            // No truncation.
            ::obake::detail::ignore(x, y, ss);

            return ::std::make_tuple();
        } else if constexpr (sizeof...(args) == 1u) {
            // Total degree truncation.
            ::obake::detail::ignore(args...);

            using d_impl = customisation::internal::series_default_degree_impl;
            using deg1_t = decltype(d_impl::d_extractor<T>{&ss}(x[0]));
            using deg2_t = decltype(d_impl::d_extractor<U>{&ss}(y[0]));

            return ::std::make_tuple(
                ::std::vector<deg1_t>(::boost::make_transform_iterator(x.cbegin(), d_impl::d_extractor<T>{&ss}),
                                      ::boost::make_transform_iterator(x.cend(), d_impl::d_extractor<T>{&ss})),
                ::std::vector<deg2_t>(::boost::make_transform_iterator(y.cbegin(), d_impl::d_extractor<U>{&ss}),
                                      ::boost::make_transform_iterator(y.cend(), d_impl::d_extractor<U>{&ss})));
        } else {
            // Partial degree truncation.
            using d_impl = customisation::internal::series_default_p_degree_impl;

            // Fetch the list of symbols from the arguments and turn it into a
            // set of indices.
            const auto &s = ::std::get<1>(::std::forward_as_tuple(args...));
            const auto si = ::obake::detail::ss_intersect_idx(s, ss);

            using deg1_t = decltype(d_impl::d_extractor<T>{&s, &si, &ss}(x[0]));
            using deg2_t = decltype(d_impl::d_extractor<U>{&s, &si, &ss}(y[0]));

            return ::std::make_tuple(
                ::std::vector<deg1_t>(
                    ::boost::make_transform_iterator(x.cbegin(), d_impl::d_extractor<T>{&s, &si, &ss}),
                    ::boost::make_transform_iterator(x.cend(), d_impl::d_extractor<T>{&s, &si, &ss})),
                ::std::vector<deg2_t>(
                    ::boost::make_transform_iterator(y.cbegin(), d_impl::d_extractor<U>{&s, &si, &ss}),
                    ::boost::make_transform_iterator(y.cend(), d_impl::d_extractor<U>{&s, &si, &ss})));
        }
    }();

    // Create vectors of indices into x and y.
    auto make_idx_vector = [](const auto &v) {
        ::std::vector<decltype(v.size())> ret;
        ret.resize(::obake::safe_cast<decltype(ret.size())>(v.size()));
        ::std::iota(ret.begin(), ret.end(), decltype(v.size())(0));
        return ret;
    };
    const auto vidx1 = make_idx_vector(x);
    const auto vidx2 = [&make_idx_vector, &y, &degree_data, &args...]() {
        ::obake::detail::ignore(args...);

        auto ret = make_idx_vector(y);

        // In truncated multiplication, order
        // the indices into y according to the degree of
        // the terms, and sort the vector of
        // degrees as well.
        if constexpr (sizeof...(args) > 0u) {
            auto &v2_deg = ::std::get<1>(degree_data);

            ::std::sort(ret.begin(), ret.end(), [&v2_deg](const auto &idx1, const auto &idx2) {
                return ::std::as_const(v2_deg)[idx1] < ::std::as_const(v2_deg)[idx2];
            });

            // Apply the permutation to v2_deg.
            ::obake::detail::container_it_diff_check(v2_deg);
            v2_deg = ::std::remove_reference_t<decltype(v2_deg)>(
                ::boost::make_permutation_iterator(v2_deg.cbegin(), ret.cbegin()),
                ::boost::make_permutation_iterator(v2_deg.cend(), ret.cend()));

            // Verify the sorting in debug mode.
            assert(::std::is_sorted(v2_deg.cbegin(), v2_deg.cend()));
        } else {
            // Nothing to do in untruncated multiplication,
            // we will just return the iota.
            ::obake::detail::ignore(y, degree_data);
        }

        return ret;
    }();

    // Parameters for the random trials.
    const auto n_trials = 20u;
    const auto multiplier = 2u;

    // NOTE: workaround for a GCC 7 issue.
    using vidx2_size_t = typename ::std::vector<decltype(y.size())>::size_type;

    // Run the trials.
    const auto c_est = ::tbb::parallel_reduce(
        ::tbb::blocked_range<unsigned>(0, n_trials), ::mppp::integer<1>{},
        [multiplier, &degree_data, &x, &y, &vidx1, &vidx2, &ss, &args...](const auto &range, ::mppp::integer<1> cur) {
            // Make a local copy of vidx1.
            auto vidx1_copy(vidx1);

            // Prepare a distribution for randomly indexing into vidx2.
            using dist_type = ::std::uniform_int_distribution<vidx2_size_t>;
            using dist_param_type = typename dist_type::param_type;
            dist_type idist;

            // Init the hash set we will be using for the trials.
            using local_set = ::absl::flat_hash_set<key_type, ::obake::detail::series_key_hasher,
                                                    ::obake::detail::series_key_comparer>;
            local_set ls;
            ls.reserve(::obake::safe_cast<decltype(ls.size())>(vidx1.size()));

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
                ::mppp::integer<1> acc_y{};

                // Temporary object for monomial multiplications.
                key_type tmp_key;

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
                                                                   // NOTE: cache as a const value,
                                                                   // so that the comparison below
                                                                   // uses const qualified values.
                                                                   const auto d_add(d1 + d2);
                                                                   return mdeg < d_add;
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
                    cur += ::mppp::integer<1>{multiplier} * count * count;
                }

                // Clear up the local set for the next iteration.
                ls.clear();
            }

            // Return the accumulated estimate.
            return cur;
        },
        [](const auto &a, const auto &b) { return a + b; });

    // Return the average of the estimates (but don't return zero).
    const auto ret = c_est / n_trials;
    if (ret.is_zero()) {
        return ::mppp::integer<1>{1};
    } else {
        return ret;
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

    // Determination of log2_nsegs.
    const auto log2_nsegs = detail::poly_mul_impl_mt_hm_compute_log2_nsegs<ret_cf_t>(v1, v2, ss);

    // Setup the number of segments in retval.
    retval.set_n_segments(log2_nsegs);

    // Cache the number of segments.
    const auto nsegs = s_size_t(1) << log2_nsegs;

    std::cout << "Total number of segments: " << nsegs << '\n';
    std::cout << "x size: " << x.size() << '\n';
    std::cout << "y size: " << y.size() << '\n';

    std::cout << "Estimated size: "
              << (v1.size() >= v2.size() ? detail::poly_mul_estimate_product_size<T, U>(v1, v2, ss, args...)
                                         : detail::poly_mul_estimate_product_size<T, U>(v2, v1, ss, args...))
              << '\n';

    // Sort the input terms according to the hash value modulo
    // 2**log2_nsegs. That is, sort them according to the bucket
    // they would occupy in a segmented table with 2**log2_nsegs
    // segments.
    // NOTE: there are parallelisation opportunities here: parallel sort,
    // computation of the segmentation in parallel, etc. Need to profile first.
    auto t_sorter = [log2_nsegs](const auto &p1, const auto &p2) {
        const auto h1 = ::obake::hash(p1.first);
        const auto h2 = ::obake::hash(p2.first);

        return h1 % (s_size_t(1) << log2_nsegs) < h2 % (s_size_t(1) << log2_nsegs);
    };
    ::std::sort(v1.begin(), v1.end(), t_sorter);
    ::std::sort(v2.begin(), v2.end(), t_sorter);

    // Compute the segmentation for the input series.
    // The segmentation is a vector of ranges (represented
    // as pairs of indices into v1/v2)
    // of size nsegs. The index at which each range is stored
    // is the index of the bucket that the terms corresponding
    // to that range would occupy in a segmented table
    // with 2**log2_nsegs segments.
    auto compute_vseg = [nsegs, log2_nsegs](const auto &v) {
        // Ensure that the size of v is representable by
        // its iterator's diff type. We need to do some
        // iterator arithmetics below.
        ::obake::detail::container_it_diff_check(v);

        using idx_t = decltype(v.size());
        ::std::vector<::std::pair<idx_t, idx_t>> vseg;
        vseg.resize(::obake::safe_cast<decltype(vseg.size())>(nsegs));

        idx_t idx = 0;
        const auto v_begin = v.begin(), v_end = v.end();
        auto it = v_begin;
        for (s_size_t i = 0; i < nsegs; ++i) {
            vseg[i].first = idx;
            it = ::std::upper_bound(it, v_end, i, [log2_nsegs](const auto &b_idx, const auto &p) {
                const auto h = ::obake::hash(p.first);
                return b_idx < h % (s_size_t(1) << log2_nsegs);
            });
            // NOTE: the overflow check was done earlier.
            idx = static_cast<idx_t>(it - v_begin);
            vseg[i].second = idx;
        }

        return vseg;
    };
    const auto vseg1 = compute_vseg(v1);
    const auto vseg2 = compute_vseg(v2);

#if !defined(NDEBUG)
    {
        // Check the segmentations in debug mode.
        assert(vseg1.size() == nsegs);
        assert(vseg2.size() == nsegs);

        decltype(v1.size()) counter1 = 0;
        decltype(v2.size()) counter2 = 0;

        auto verify_seg = [log2_nsegs](const auto &v, s_size_t bucket_idx, const auto &range, auto &counter) {
            for (auto idx = range.first; idx != range.second; ++idx) {
                const auto h = ::obake::hash(v[idx].first);
                assert(h % (s_size_t(1) << log2_nsegs) == bucket_idx);
                ++counter;
            }
        };

        for (s_size_t i = 0; i < nsegs; ++i) {
            if (i) {
                // Ensure the current range begins
                // where the previous range ends.
                assert(vseg1[i].first == vseg1[i - 1u].second);
                assert(vseg2[i].first == vseg2[i - 1u].second);
            }
            verify_seg(v1, i, vseg1[i], counter1);
            verify_seg(v2, i, vseg2[i], counter2);
        }

        assert(counter1 == v1.size());
        assert(counter2 == v2.size());
    }
#endif

    // Functor to compute the end index in the
    // inner multiplication loops below, given
    // an index into the first series and an index
    // range into the second series. In non-truncated
    // mode, this functor will always return the end
    // of the range, otherwise the returned
    // value will ensure that the truncation limits
    // are respected.
    auto compute_end_idx2 = [&ss, &v1, &v2, &vseg1, &vseg2, &args...]() {
        if constexpr (sizeof...(args) == 0u) {
            ::obake::detail::ignore(ss, v1, v2, vseg1, vseg2);

            return [](const auto &, const auto &r2) { return r2.second; };
        } else {
            // Helper that, given a list of ranges vseg into v, will:
            //
            // - create and return a vector vd
            //   containing the total/partial degrees of all the
            //   terms in v, with the degrees within each vseg range sorted
            //   in ascending order,
            // - sort v according to vd.
            //
            // v will be one of v1/v2, t is a type_c instance
            // containing either T or U.
            auto sorter = [&ss, &args...](auto &v, auto t, const auto &vseg) {
                // NOTE: we will be using the machinery from the default implementation
                // of degree() for series, so that we can re-use the concept checking bits
                // as well.
                using s_t = typename decltype(t)::type;

                // Compute the vector of degrees.
                auto vd = [&v, &ss, &args...]() {
                    if constexpr (sizeof...(args) == 1u) {
                        // Total degree.
                        using d_impl = customisation::internal::series_default_degree_impl;
                        using deg_t = decltype(d_impl::d_extractor<s_t>{&ss}(*v.cbegin()));

                        ::obake::detail::ignore(args...);

                        return ::std::vector<deg_t>(
                            ::boost::make_transform_iterator(v.cbegin(), d_impl::d_extractor<s_t>{&ss}),
                            ::boost::make_transform_iterator(v.cend(), d_impl::d_extractor<s_t>{&ss}));
                    } else {
                        // Partial degree.
                        using d_impl = customisation::internal::series_default_p_degree_impl;

                        // Fetch the list of symbols from the arguments and turn it into a
                        // set of indices.
                        const auto &s = ::std::get<1>(::std::forward_as_tuple(args...));
                        const auto si = ::obake::detail::ss_intersect_idx(s, ss);

                        using deg_t = decltype(d_impl::d_extractor<s_t>{&s, &si, &ss}(*v.cbegin()));

                        return ::std::vector<deg_t>(
                            ::boost::make_transform_iterator(v.cbegin(), d_impl::d_extractor<s_t>{&s, &si, &ss}),
                            ::boost::make_transform_iterator(v.cend(), d_impl::d_extractor<s_t>{&s, &si, &ss}));
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

                // Sort indirectly each range from vseg according to the degree.
                // NOTE: capture vd as const ref because in the lt-comparable requirements for the degree
                // type we are using const lrefs.
                for (const auto &r : vseg) {
                    ::std::sort(vidx.data() + r.first, vidx.data() + r.second,
                                [&vdc = ::std::as_const(vd)](const auto &idx1, const auto &idx2) {
                                    return vdc[idx1] < vdc[idx2];
                                });
                }

                // Apply the sorting to vd and v. Ensure we don't run
                // into overflows during the permutated access.
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
                for (const auto &r : vseg) {
                    const auto &idx_begin = r.first;
                    const auto &idx_end = r.second;

                    // NOTE: add constness to vd in order to ensure that
                    // the degrees are compared via const refs.
                    assert(
                        ::std::is_sorted(::std::as_const(vd).data() + idx_begin, ::std::as_const(vd).data() + idx_end));

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

                        assert(::std::equal(vd.data() + idx_begin, vd.data() + idx_end,
                                            ::boost::make_transform_iterator(v.data() + idx_begin,
                                                                             d_impl::d_extractor<s_t>{&s, &si, &ss}),
                                            [](const auto &a, const auto &b) { return !(a < b) && !(b < a); }));
                    }
                }
#endif

                return vd;
            };

            using ::obake::detail::type_c;
            return [vd1 = sorter(v1, type_c<T>{}, vseg1), vd2 = sorter(v2, type_c<U>{}, vseg2),
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
                const auto it = ::std::upper_bound(vd2.cbegin() + static_cast<it_diff_t>(r2.first),
                                                   vd2.cbegin() + static_cast<it_diff_t>(r2.second), max_deg,
                                                   [&d_i](const auto &mdeg, const auto &d_j) {
                                                       // NOTE: cache as a const value,
                                                       // so that the comparison below
                                                       // uses const qualified values.
                                                       const auto d_add(d_i + d_j);
                                                       return mdeg < d_add;
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

    try {
        // Parallel iteration over the number of buckets of the
        // output segmented table.
        ::tbb::parallel_for(::tbb::blocked_range<s_size_t>(0, nsegs), [&v1, &v2, &vseg1, &vseg2, nsegs, log2_nsegs,
                                                                       &retval, &ss, mts = retval._get_max_table_size(),
                                                                       &compute_end_idx2
#if !defined(NDEBUG)
                                                                       ,
                                                                       &n_mults
#endif
        ](const auto &range) {
            // Cache the pointers to the terms data.
            // NOTE: doing it here rather than in the lambda
            // capture seems to help performance on GCC.
            auto vptr1 = v1.data();
            auto vptr2 = v2.data();

            // Temporary variable used in monomial multiplication.
            ret_key_t tmp_key;

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
                    const auto j = seg_idx >= i ? (seg_idx - i) : ((s_size_t(1) << log2_nsegs) - i + seg_idx);
                    assert(j < vseg2.size());

                    // Fetch the corresponding ranges.
                    const auto r1 = vseg1[i];
                    const auto r2 = vseg2[j];

                    // The O(N**2) multiplication loop over the ranges.
                    const auto idx_end1 = r1.second;
                    for (auto idx1 = r1.first; idx1 != idx_end1; ++idx1) {
                        const auto &[k1, c1] = *(vptr1 + idx1);

                        const auto end2 = vptr2 + compute_end_idx2(idx1, r2);
                        for (auto ptr2 = vptr2 + r2.first; ptr2 != end2; ++ptr2) {
                            const auto &[k2, c2] = *ptr2;

                            // Do the monomial multiplication.
                            ::obake::monomial_mul(tmp_key, k1, k2, ss);

                            // Check that the result ends up in the correct bucket.
                            assert(::obake::hash(tmp_key) % (s_size_t(1) << log2_nsegs) == seg_idx);

                            // Attempt the insertion. The coefficients are lazily multiplied
                            // only if the insertion actually takes place.
                            const auto res
                                = table.try_emplace(tmp_key, poly_cf_mul_expr<cf1_t, cf2_t, ret_cf_t>{c1, c2});

                            // NOTE: optimise with likely/unlikely here?
                            if (!res.second) {
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
        });

#if !defined(NDEBUG)
        // Verify the number of term multiplications we performed,
        // but only if we are in non-truncated mode.
        if constexpr (sizeof...(args) == 0u) {
            assert(n_mults.load()
                   == static_cast<unsigned long long>(x.size()) * static_cast<unsigned long long>(y.size()));
        }
#endif
    } catch (...) {
        // LCOV_EXCL_START
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
                        using d_impl = customisation::internal::series_default_degree_impl;
                        using deg_t = decltype(d_impl::d_extractor<s_t>{&ss}(*v.cbegin()));

                        ::obake::detail::ignore(args...);

                        return ::std::vector<deg_t>(
                            ::boost::make_transform_iterator(v.cbegin(), d_impl::d_extractor<s_t>{&ss}),
                            ::boost::make_transform_iterator(v.cend(), d_impl::d_extractor<s_t>{&ss}));
                    } else {
                        // Partial degree.
                        using d_impl = customisation::internal::series_default_p_degree_impl;

                        // Fetch the list of symbols from the arguments and turn it into a
                        // set of indices.
                        const auto &s = ::std::get<1>(::std::forward_as_tuple(args...));
                        const auto si = ::obake::detail::ss_intersect_idx(s, ss);

                        using deg_t = decltype(d_impl::d_extractor<s_t>{&s, &si, &ss}(*v.cbegin()));

                        return ::std::vector<deg_t>(
                            ::boost::make_transform_iterator(v.cbegin(), d_impl::d_extractor<s_t>{&s, &si, &ss}),
                            ::boost::make_transform_iterator(v.cend(), d_impl::d_extractor<s_t>{&s, &si, &ss}));
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
                          // NOTE: cache as a const value,
                          // so that the comparison below
                          // uses const qualified values.
                          const auto d_add(d_i + d_j);
                          return mdeg < d_add;
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
        ret_key_t tmp_key;

        const auto v1_size = v1.size();
        for (decltype(v1.size()) i = 0; i < v1_size; ++i) {
            const auto &t1 = v1[i];
            const auto &k1 = t1->first;
            const auto &c1 = t1->second;

            // Get the upper limit of the multiplication
            // range in v2.
            const auto j_end = compute_j_end(i);
            if (j_end == 0u) {
                // If j_end is zero, we don't need to perform
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
                const auto res = tab.try_emplace(tmp_key, poly_cf_mul_expr<cf1_t, cf2_t, ret_cf_t>{c1, c2});

                // NOTE: optimise with likely/unlikely here?
                if (!res.second) {
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
    } catch (...) {
        // LCOV_EXCL_START
        // retval may now contain zero coefficients.
        // Make sure to clear it before rethrowing.
        tab.clear();
        throw;
        // LCOV_EXCL_STOP
    }
}

// Implementation of poly multiplication with identical symbol sets.
template <typename T, typename U, typename... Args>
inline auto poly_mul_impl_identical_ss(T &&x, U &&y, const Args &... args)
{
    using ret_t = poly_mul_ret_t<T &&, U &&>;
    using ret_key_t = series_key_t<ret_t>;

    assert(x.get_symbol_set() == y.get_symbol_set());

    // Init the return value.
    ret_t retval;
    retval.set_symbol_set(x.get_symbol_set());

    if (x.empty() || y.empty()) {
        // Exit early if either series is empty.
        return retval;
    }

    if constexpr (::std::conjunction_v<is_homomorphically_hashable_monomial<ret_key_t>,
                                       // Need also to be able to measure the byte size
                                       // of the key/cf of ret_t, via const lvalue references.
                                       // NOTE: perhaps this is too much of a hard requirement,
                                       // and we can make this optional (if not supported,
                                       // fix the nsegs to something like twice the cores).
                                       is_size_measurable<const ret_key_t &>,
                                       is_size_measurable<const series_cf_t<ret_t> &>>) {
        // Homomorphic hashing is available, we can run
        // the multi-threaded implementation.
        // NOTE: perhaps the heuristic here can be improved
        // by taking into account the byte sizes of x/y?
        if (x.size() < 1000u / y.size() || ::obake::detail::hc() == 1u) {
            // If the operands are "small" (less than N
            // term-by-term mults), or we just have 1 core
            // available, just run the simple
            // implementation.
            detail::poly_mul_impl_simple(retval, x, y, args...);
        } else {
            // "Large" operands and multiple cores available,
            // run the MT implementation.
            detail::poly_mul_impl_mt_hm(retval, x, y, args...);
        }
    } else {
        // The monomial does not have homomorphic hashing,
        // just use the simple implementation.
        detail::poly_mul_impl_simple(retval, x, y, args...);
    }

    return retval;
}

// Top level function for poly multiplication.
template <typename T, typename U, typename... Args>
inline auto poly_mul_impl(T &&x, U &&y, const Args &... args)
{
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

} // namespace detail

template <typename T, typename U, ::std::enable_if_t<detail::poly_mul_algo<T &&, U &&> != 0, int> = 0>
inline detail::poly_mul_ret_t<T &&, U &&> series_mul(T &&x, U &&y)
{
    return detail::poly_mul_impl(::std::forward<T>(x), ::std::forward<U>(y));
}

namespace detail
{

// Metaprogramming to establish if we can perform
// truncated total/partial degree multiplication on the
// polynomial operands T and U with degree limit of type V.
template <typename T, typename U, typename V, bool Total>
constexpr auto poly_mul_truncated_degree_algorithm_impl()
{
    // Check first if we can do the untruncated multiplication. If we cannot,
    // truncated mult is not possible.
    if constexpr (poly_mul_algo<T, U> == 0) {
        return 0;
    } else {
        using d_impl = ::std::conditional_t<Total, customisation::internal::series_default_degree_impl,
                                            customisation::internal::series_default_p_degree_impl>;

        // Check if we can compute the degree of the terms via the default
        // implementation for series.
        constexpr auto algo1 = d_impl::template algo<T>;
        constexpr auto algo2 = d_impl::template algo<U>;

        if constexpr (algo1 != 0 && algo2 != 0) {
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

            if constexpr (::std::conjunction_v<
                              // NOTE: verify deg_add_t, before using it below.
                              is_detected<::obake::detail::add_t, const deg1_t &, const deg2_t &>,
                              ::std::is_copy_constructible<deg1_t>, ::std::is_copy_constructible<deg2_t>,
                              ::std::is_move_constructible<deg1_t>, ::std::is_move_constructible<deg2_t>,
                              is_less_than_comparable<::std::add_lvalue_reference_t<const V>,
                                                      ::std::add_lvalue_reference_t<const deg_add_t>>>) {
                return 1;
            } else {
                return 0;
            }
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
template <typename T, typename U, typename V,
          ::std::enable_if_t<detail::poly_mul_truncated_degree_algo<T &&, U &&, V> != 0, int> = 0>
inline detail::poly_mul_ret_t<T &&, U &&> truncated_mul(T &&x, U &&y, const V &max_degree)
{
    return detail::poly_mul_impl(::std::forward<T>(x), ::std::forward<U>(y), max_degree);
}

template <typename T, typename U, typename V,
          ::std::enable_if_t<detail::poly_mul_truncated_p_degree_algo<T &&, U &&, V> != 0, int> = 0>
inline detail::poly_mul_ret_t<T &&, U &&> truncated_mul(T &&x, U &&y, const V &max_degree, const symbol_set &s)
{
    return detail::poly_mul_impl(::std::forward<T>(x), ::std::forward<U>(y), max_degree, s);
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
                              is_compound_addable<::std::add_lvalue_reference_t<ret_t>, ret_t>,
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
constexpr auto poly_truncate_degree_algorithm_impl()
{
    using rT = remove_cvref_t<T>;

    // Shortcut for signalling that the truncate_degree() implementation
    // is not well-defined.
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, ::obake::detail::type_c<void>{});

    if constexpr (!is_polynomial_v<rT>) {
        // Not a polynomial.
        return failure;
    } else {
        using key_t = series_key_t<rT>;

        if constexpr (is_key_with_degree_v<const key_t &>) {
            // The key supports degree computation.

            if constexpr (::std::disjunction_v<is_with_degree<const series_cf_t<rT> &>>) {
                // NOTE: for the time being, we deal with only truncation
                // based on key filtering. Thus, if the coefficient type
                // has a degree, return failure.
                return failure;
            } else {
                // The truncation will involve only key-based
                // filtering. We need to be able to lt-compare U
                // to the degree type of the key (const lvalue
                // ref vs rvalue). The degree of the key
                // is computed via lvalue ref.
                if constexpr (is_less_than_comparable_v<::std::add_lvalue_reference_t<const remove_cvref_t<U>>,
                                                        ::obake::detail::key_degree_t<const key_t &>>) {
                    return ::std::make_pair(1, ::obake::detail::type_c<rT>{});
                } else {
                    return failure;
                }
            }
        } else {
            // The key does not support degree computation, fail.
            // NOTE: the case in which the coefficient is degree
            // truncatable and the key does not support degree computation
            // should eventually be handled in a default series implementation
            // of truncate_degree().
            return failure;
        }
    }
}

template <typename T, typename U>
inline constexpr auto poly_truncate_degree_algorithm = detail::poly_truncate_degree_algorithm_impl<T, U>();

template <typename T, typename U>
inline constexpr int poly_truncate_degree_algo = poly_truncate_degree_algorithm<T, U>.first;

template <typename T, typename U>
using poly_truncate_degree_ret_t = typename decltype(poly_truncate_degree_algorithm<T, U>.second)::type;

} // namespace detail

template <typename T, typename U, ::std::enable_if_t<detail::poly_truncate_degree_algo<T &&, U &&> != 0, int> = 0>
inline detail::poly_truncate_degree_ret_t<T &&, U &&> truncate_degree(T &&x_, U &&y_)
{
    using ret_t = detail::poly_truncate_degree_ret_t<T &&, U &&>;
    constexpr auto algo = detail::poly_truncate_degree_algo<T &&, U &&>;

    // Sanity checks.
    static_assert(::std::is_same_v<ret_t, remove_cvref_t<T>>);
    static_assert(algo == 1);

    // We will need only const access to x and y.
    const auto &x = ::std::as_const(x_);
    const auto &y = ::std::as_const(y_);

    // Cache the symbol set.
    const auto &ss = x.get_symbol_set();

    // Prepare the return value: same symbol set,
    // same number of segments, but don't reserve
    // space beforehand.
    ret_t retval;
    retval.set_symbol_set(ss);
    retval.set_n_segments(x.get_s_size());

    // Get references to the in & out tables.
    const auto &in_s_table = x._get_s_table();
    auto &out_s_table = retval._get_s_table();

    // NOTE: parallelisation opportunities here,
    // since we operate table by table.
    // NOTE: in principle we could exploit an x rvalue
    // here to move the existing coefficients instead
    // of copying them (as we do elsewhere with the help
    // of a rref_cleaner). Keep it in mind for the
    // future.
    for (decltype(in_s_table.size()) i = 0; i < in_s_table.size(); ++i) {
        const auto &tab_in = in_s_table[i];
        auto &tab_out = out_s_table[i];

        for (const auto &t : tab_in) {
            const auto &k = t.first;

            if (!(y < ::obake::key_degree(k, ss))) {
                // The key degree does not exceed the
                // limit, add the term to the return value.
                // NOTE: we can emplace directly into the table
                // with no checks, as we are not changing anything
                // in the term.
                [[maybe_unused]] const auto res = tab_out.try_emplace(k, t.second);
                assert(res.second);
            }
        }
    }

    return retval;
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
                                  // as nonesuch is not compound addable.
                                  is_compound_addable<::std::add_lvalue_reference_t<s_t>, s_t>,
                                  // Need also to add in place with prod2_t in case
                                  // the differentiation variable is not in the polynomial.
                                  is_compound_addable<::std::add_lvalue_reference_t<s_t>, prod2_t>,
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
                                  //  Ensure quot_t is detected, as we use it in the definition of ret_t.
                                  is_detected<::obake::detail::div_t, const cf_t &, key_int_t>,
                                  // The return type must be accumulable.
                                  // NOTE: this condition also checks that ret_t is detected,
                                  // as nonesuch is not compound addable.
                                  is_compound_addable<::std::add_lvalue_reference_t<ret_t>, ret_t>,
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
