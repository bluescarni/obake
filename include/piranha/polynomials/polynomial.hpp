// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP
#define PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/iterator/permutation_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#include <mp++/integer.hpp>

#include <piranha/byte_size.hpp>
#include <piranha/config.hpp>
#include <piranha/detail/container_it_diff_check.hpp>
#include <piranha/detail/hc.hpp>
#include <piranha/detail/ignore.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/detail/type_c.hpp>
#include <piranha/detail/xoroshiro128_plus.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/hash.hpp>
#include <piranha/key/key_merge_symbols.hpp>
#include <piranha/math/degree.hpp>
#include <piranha/math/fma3.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/math/safe_cast.hpp>
#include <piranha/polynomials/monomial_homomorphic_hash.hpp>
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

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Polynomial = is_polynomial_v<T>;

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
        ::std::vector<int> tmp(::piranha::safe_cast<::std::vector<int>::size_type>(ss.size()));
        const auto it = ::std::lower_bound(ss.begin(), ss.end(), s);
        if (piranha_unlikely(it == ss.end() || *it != s)) {
            piranha_throw(::std::invalid_argument, "Cannot create a polynomial with symbol set " + detail::to_string(ss)
                                                       + " from the generator '" + s
                                                       + "': the generator is not in the symbol set");
        }

        // Set to 1 the exponent of the corresponding generator.
        tmp[static_cast<::std::vector<int>::size_type>(ss.index_of(it))] = 1;

        // Create and add a new term.
        retval.add_term(
            series_key_t<T>(static_cast<const int *>(tmp.data()), static_cast<const int *>(tmp.data() + tmp.size())),
            1);

        return retval;
    };

    return ::std::array<T, sizeof...(Args)>{make_poly(names)...};
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

    return ::std::array<T, sizeof...(Args)>{make_poly(names)...};
}

} // namespace detail

#if defined(_MSC_VER)

template <typename T>
struct make_polynomials_msvc {
    template <typename... Args>
    constexpr auto operator()(const Args &... args) const
        PIRANHA_SS_FORWARD_MEMBER_FUNCTION(detail::make_polynomials_impl<T>(args...))
};

template <typename T>
inline constexpr auto make_polynomials = make_polynomials_msvc<T>{};

#else

// Polynomial creation functor.
template <typename T>
inline constexpr auto make_polynomials
    = [](const auto &... args) PIRANHA_SS_FORWARD_LAMBDA(detail::make_polynomials_impl<T>(args...));

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
            // Determine if the cf/key types support all the necessary
            // bits.
            using cf1_t = series_cf_t<rT>;
            using cf2_t = series_cf_t<rU>;
            using ret_cf_t = detected_t<::piranha::detail::mul_t, const cf1_t &, const cf2_t &>;

            if constexpr (::std::conjunction_v<
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
    ::piranha::detail::xoroshiro128_plus rng{s1 + static_cast<::std::uint64_t>(v1.size()),
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
        ::piranha::monomial_mul(tmp_key, v1[idx1].first, v2[idx2].first, ss);
        const auto tmp_cf = v1[idx1].second * v2[idx2].second;

        // Accumulate the size of the produced term: size of monomial,
        // coefficient, and, if present, padding.
        acc += ::piranha::byte_size(static_cast<const ret_key_t &>(tmp_key)) + ::piranha::byte_size(tmp_cf) + pad_size;
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
    const auto est_total_size = 5. / 100. * (avg_size * static_cast<double>(v1_size) * static_cast<double>(v2_size));

    // Compute the number of segments by enforcing a fixed
    // amount of bytes per segment.
    const auto nsegs
        = ::boost::numeric_cast<typename polynomial<ret_key_t, RetCf>::s_size_type>(est_total_size / (500. * 1024.));

    // Finally, compute the log2 + 1 of nsegs and return it, but
    // make sure that it is not greater than the max_log2_size()
    // allowed in a series.
    // NOTE: even if nsegs is zero, this will still yield some
    // useful value, as nbits() on zero will return zero.
    return ::std::min(::piranha::safe_cast<unsigned>(::mppp::integer<1>{nsegs}.nbits()),
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

// Add an input value y to another value x, a reference
// to which is created on construction.
template <typename T>
struct poly_mul_impl_degree_adder {
    // Ensure def-constructability.
    poly_mul_impl_degree_adder() : x_ptr(nullptr) {}
    explicit poly_mul_impl_degree_adder(const T *ptr) : x_ptr(ptr) {}
    template <typename U>
    auto operator()(const U &y) const
    {
        assert(x_ptr != nullptr);

        return *x_ptr + y;
    }
    const T *x_ptr;
};

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
        = ::piranha::detail::make_range(::boost::make_transform_iterator(v1.cbegin(), poly_term_key_ref_extractor{}),
                                        ::boost::make_transform_iterator(v1.cend(), poly_term_key_ref_extractor{}));
    const auto r2
        = ::piranha::detail::make_range(::boost::make_transform_iterator(v2.cbegin(), poly_term_key_ref_extractor{}),
                                        ::boost::make_transform_iterator(v2.cend(), poly_term_key_ref_extractor{}));
    if constexpr (are_overflow_testable_monomial_ranges_v<decltype(r1) &, decltype(r2) &>) {
        // Do the monomial overflow checking.
        if (piranha_unlikely(!::piranha::monomial_range_overflow_check(r1, r2, ss))) {
            piranha_throw(
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

    // Sort the input terms according to the hash value modulo
    // 2**log2_nsegs. That is, sort them according to the bucket
    // they would occupy in a segmented table with 2**log2_nsegs
    // segments.
    // NOTE: there are parallelisation opportunities here: parallel sort,
    // computation of the segmentation in parallel, etc. Need to profile first.
    auto t_sorter = [log2_nsegs](const auto &p1, const auto &p2) {
        const auto h1 = ::piranha::hash(p1.first);
        const auto h2 = ::piranha::hash(p2.first);

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
        ::piranha::detail::container_it_diff_check(v);

        using idx_t = decltype(v.size());
        ::std::vector<::std::pair<idx_t, idx_t>> vseg;
        vseg.resize(::piranha::safe_cast<decltype(vseg.size())>(nsegs));

        idx_t idx = 0;
        const auto v_begin = v.begin(), v_end = v.end();
        auto it = v_begin;
        for (s_size_t i = 0; i < nsegs; ++i) {
            vseg[i].first = idx;
            it = ::std::upper_bound(it, v_end, i, [log2_nsegs](const auto &b_idx, const auto &p) {
                const auto h = ::piranha::hash(p.first);
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
                const auto h = ::piranha::hash(v[idx].first);
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
            ::piranha::detail::ignore(ss, v1, v2, vseg1, vseg2);

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
                        using d_impl = ::piranha::customisation::internal::series_default_degree_impl;
                        using deg_t = decltype(d_impl::d_extractor<s_t>{&ss}(*v.cbegin()));

                        ::piranha::detail::ignore(args...);

                        return ::std::vector<deg_t>(
                            ::boost::make_transform_iterator(v.cbegin(), d_impl::d_extractor<s_t>{&ss}),
                            ::boost::make_transform_iterator(v.cend(), d_impl::d_extractor<s_t>{&ss}));
                    } else {
                        // Partial degree.
                        using d_impl = ::piranha::customisation::internal::series_default_p_degree_impl;

                        // Fetch the list of symbols from the arguments and turn it into a
                        // set of indices.
                        const auto &s = ::std::get<1>(::std::forward_as_tuple(args...));
                        const auto si = ::piranha::detail::ss_intersect_idx(s, ss);

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
                ::piranha::detail::container_it_diff_check(static_cast<const decltype(vd) &>(vd));

                // Create a vector of indices into vd.
                ::std::vector<decltype(vd.size())> vidx;
                vidx.resize(::piranha::safe_cast<decltype(vidx.size())>(vd.size()));
                ::std::iota(vidx.begin(), vidx.end(), decltype(vd.size())(0));

                // Sort indirectly each range from vseg according to the total degree.
                for (const auto &[idx_begin, idx_end] : vseg) {
                    ::std::sort(vidx.data() + idx_begin, vidx.data() + idx_end,
                                [&vd](const auto &idx1, const auto &idx2) { return vd[idx1] < vd[idx2]; });
                }

                // Apply the sorting to vd and v. Ensure we don't run
                // into overflows during the permutated access.
                ::piranha::detail::container_it_diff_check(vd);
                vd = decltype(vd)(::boost::make_permutation_iterator(vd.begin(), vidx.begin()),
                                  ::boost::make_permutation_iterator(vd.end(), vidx.end()));
                ::piranha::detail::container_it_diff_check(v);
                v = ::std::remove_reference_t<decltype(v)>(::boost::make_permutation_iterator(v.begin(), vidx.begin()),
                                                           ::boost::make_permutation_iterator(v.end(), vidx.end()));

#if !defined(NDEBUG)
                // Check the results in debug mode.
                for (const auto &[idx_begin, idx_end] : vseg) {
                    assert(::std::is_sorted(vd.data() + idx_begin, vd.data() + idx_end));

                    if constexpr (sizeof...(args) == 1u) {
                        using d_impl = ::piranha::customisation::internal::series_default_degree_impl;

                        assert(::std::equal(
                            vd.data() + idx_begin, vd.data() + idx_end,
                            ::boost::make_transform_iterator(v.data() + idx_begin, d_impl::d_extractor<s_t>{&ss}),
                            [](const auto &a, const auto &b) { return a == b; }));
                    } else {
                        using d_impl = ::piranha::customisation::internal::series_default_p_degree_impl;

                        const auto &s = ::std::get<1>(::std::forward_as_tuple(args...));
                        const auto si = ::piranha::detail::ss_intersect_idx(s, ss);

                        assert(::std::equal(vd.data() + idx_begin, vd.data() + idx_end,
                                            ::boost::make_transform_iterator(v.data() + idx_begin,
                                                                             d_impl::d_extractor<s_t>{&s, &si, &ss}),
                                            [](const auto &a, const auto &b) { return a == b; }));
                    }
                }
#endif

                return vd;
            };

            using ::piranha::detail::type_c;
            return [vd1 = sorter(v1, type_c<T>{}, vseg1), vd2 = sorter(v2, type_c<U>{}, vseg2),
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
                const auto it = ::std::upper_bound(
                    ::boost::make_transform_iterator(vd2.cbegin() + static_cast<it_diff_t>(r2.first),
                                                     poly_mul_impl_degree_adder(&d_i)),
                    ::boost::make_transform_iterator(vd2.cbegin() + static_cast<it_diff_t>(r2.second),
                                                     poly_mul_impl_degree_adder(&d_i)),
                    max_deg);

                // Turn the iterator into an index and return it.
                // NOTE: we checked above that the iterator diff
                // type can safely be used as an index (for both
                // vd1 and vd2).
                return static_cast<idx_t>(it.base() - vd2.cbegin());
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
        ::tbb::parallel_for(::tbb::blocked_range(s_size_t(0), nsegs), [&v1, &v2, &vseg1, &vseg2, nsegs, log2_nsegs,
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
                            ::piranha::monomial_mul(tmp_key, k1, k2, ss);

                            // Check that the result ends up in the correct bucket.
                            assert(::piranha::hash(tmp_key) % (s_size_t(1) << log2_nsegs) == seg_idx);

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
                                    ::piranha::fma3(res.first->second, c1, c2);
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
                    auto tmp_it = it++;
                    if (piranha_unlikely(::piranha::is_zero(static_cast<const ret_cf_t &>(tmp_it->second)))) {
                        table.erase(tmp_it);
                    }
                }

                // LCOV_EXCL_START
                // Check the table size against the max allowed size.
                if (piranha_unlikely(table.size() > mts)) {
                    piranha_throw(::std::overflow_error, "The homomorphic multithreaded multiplication of two "
                                                         "polynomials resulted in a table whose size ("
                                                             + ::piranha::detail::to_string(table.size())
                                                             + ") is larger than the maximum allowed value ("
                                                             + ::piranha::detail::to_string(mts) + ")");
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
        = ::piranha::detail::make_range(::boost::make_transform_iterator(v1.cbegin(), poly_term_key_ref_extractor{}),
                                        ::boost::make_transform_iterator(v1.cend(), poly_term_key_ref_extractor{}));
    const auto r2
        = ::piranha::detail::make_range(::boost::make_transform_iterator(v2.cbegin(), poly_term_key_ref_extractor{}),
                                        ::boost::make_transform_iterator(v2.cend(), poly_term_key_ref_extractor{}));
    if constexpr (are_overflow_testable_monomial_ranges_v<decltype(r1) &, decltype(r2) &>) {
        // Do the monomial overflow checking.
        if (piranha_unlikely(!::piranha::monomial_range_overflow_check(r1, r2, ss))) {
            piranha_throw(
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
            ::piranha::detail::ignore(v1, ss);

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
                        using d_impl = ::piranha::customisation::internal::series_default_degree_impl;
                        using deg_t = decltype(d_impl::d_extractor<s_t>{&ss}(*v.cbegin()));

                        ::piranha::detail::ignore(args...);

                        return ::std::vector<deg_t>(
                            ::boost::make_transform_iterator(v.cbegin(), d_impl::d_extractor<s_t>{&ss}),
                            ::boost::make_transform_iterator(v.cend(), d_impl::d_extractor<s_t>{&ss}));
                    } else {
                        // Partial degree.
                        using d_impl = ::piranha::customisation::internal::series_default_p_degree_impl;

                        // Fetch the list of symbols from the arguments and turn it into a
                        // set of indices.
                        const auto &s = ::std::get<1>(::std::forward_as_tuple(args...));
                        const auto si = ::piranha::detail::ss_intersect_idx(s, ss);

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
                ::piranha::detail::container_it_diff_check(static_cast<const decltype(vd) &>(vd));

                // Create a vector of indices into vd.
                ::std::vector<decltype(vd.size())> vidx;
                vidx.resize(::piranha::safe_cast<decltype(vidx.size())>(vd.size()));
                ::std::iota(vidx.begin(), vidx.end(), decltype(vd.size())(0));

                // Sort indirectly.
                ::std::sort(vidx.begin(), vidx.end(),
                            [&vd](const auto &idx1, const auto &idx2) { return vd[idx1] < vd[idx2]; });

                // Apply the sorting to vd and v. Check that permutated
                // access does not result in overflow.
                ::piranha::detail::container_it_diff_check(vd);
                vd = decltype(vd)(::boost::make_permutation_iterator(vd.begin(), vidx.begin()),
                                  ::boost::make_permutation_iterator(vd.end(), vidx.end()));
                ::piranha::detail::container_it_diff_check(v);
                v = ::std::remove_reference_t<decltype(v)>(::boost::make_permutation_iterator(v.begin(), vidx.begin()),
                                                           ::boost::make_permutation_iterator(v.end(), vidx.end()));

#if !defined(NDEBUG)
                // Check the results in debug mode.
                assert(::std::is_sorted(vd.begin(), vd.end()));

                if constexpr (sizeof...(args) == 1u) {
                    using d_impl = ::piranha::customisation::internal::series_default_degree_impl;

                    assert(::std::equal(vd.begin(), vd.end(),
                                        ::boost::make_transform_iterator(v.cbegin(), d_impl::d_extractor<s_t>{&ss}),
                                        [](const auto &a, const auto &b) { return a == b; }));
                } else {
                    using d_impl = ::piranha::customisation::internal::series_default_p_degree_impl;

                    const auto &s = ::std::get<1>(::std::forward_as_tuple(args...));
                    const auto si = ::piranha::detail::ss_intersect_idx(s, ss);

                    assert(::std::equal(
                        vd.begin(), vd.end(),
                        ::boost::make_transform_iterator(v.cbegin(), d_impl::d_extractor<s_t>{&s, &si, &ss}),
                        [](const auto &a, const auto &b) { return a == b; }));
                }
#endif

                return vd;
            };

            using ::piranha::detail::type_c;
            return [vd1 = sorter(v1, type_c<T>{}), vd2 = sorter(v2, type_c<U>{}),
                    &max_deg = ::std::get<0>(::std::forward_as_tuple(args...))](const auto &i) {
                using idx_t = remove_cvref_t<decltype(i)>;

                // Get the total/partial degree of the current term
                // in the first series.
                const auto &d_i = vd1[i];

                // Find the first term in the second series such
                // that d_i + d_j > max_deg.
                const auto it = ::std::upper_bound(
                    ::boost::make_transform_iterator(vd2.cbegin(), poly_mul_impl_degree_adder(&d_i)),
                    ::boost::make_transform_iterator(vd2.cend(), poly_mul_impl_degree_adder(&d_i)), max_deg);

                // Turn the iterator into an index and return it.
                // NOTE: we checked above that the iterator diff
                // type can safely be used as an index (for both
                // vd1 and vd2).
                return static_cast<idx_t>(it.base() - vd2.cbegin());
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
                ::piranha::monomial_mul(tmp_key, k1, t2->first, ss);

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
        const auto it_f = tab.end();
        for (auto it = tab.begin(); it != it_f;) {
            // NOTE: abseil's flat_hash_map returns void on erase(),
            // thus we need to increase 'it' before possibly erasing.
            // erase() does not cause rehash and thus will not invalidate
            // any other iterator apart from the one being erased.
            auto tmp_it = it++;
            if (piranha_unlikely(::piranha::is_zero(static_cast<const ret_cf_t &>(tmp_it->second)))) {
                tab.erase(tmp_it);
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
        if (x.size() < 1000u / y.size() || ::piranha::detail::hc() == 1u) {
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

                return detail::poly_mul_impl_identical_ss(::std::forward<T>(x), ::std::move(b), args...);
            }
            case 2u: {
                // y already has the correct symbol
                // set, extend only x.
                rT a;
                a.set_symbol_set(merged_ss);
                ::piranha::detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);

                return detail::poly_mul_impl_identical_ss(::std::move(a), ::std::forward<U>(y), args...);
            }
        }

        // Both x and y need to be extended.
        rT a;
        rU b;
        a.set_symbol_set(merged_ss);
        b.set_symbol_set(merged_ss);
        ::piranha::detail::series_sym_extender(a, ::std::forward<T>(x), ins_map_x);
        ::piranha::detail::series_sym_extender(b, ::std::forward<U>(y), ins_map_y);

        return detail::poly_mul_impl_identical_ss(::std::move(a), ::std::move(b), args...);
    }
}

} // namespace detail

template <typename T, typename U, ::std::enable_if_t<detail::poly_mul_algo<T &&, U &&> != 0, int> = 0>
inline detail::poly_mul_ret_t<T &&, U &&> series_mul(T &&x, U &&y)
{
    return detail::poly_mul_impl(::std::forward<T>(x), ::std::forward<U>(y));
}

template <typename T, typename U, typename V, ::std::enable_if_t<detail::poly_mul_algo<T &&, U &&> != 0, int> = 0>
inline detail::poly_mul_ret_t<T &&, U &&> truncated_mul(T &&x, U &&y, const V &max_degree)
{
    return detail::poly_mul_impl(::std::forward<T>(x), ::std::forward<U>(y), max_degree);
}

template <typename T, typename U, typename V, ::std::enable_if_t<detail::poly_mul_algo<T &&, U &&> != 0, int> = 0>
inline detail::poly_mul_ret_t<T &&, U &&> truncated_mul(T &&x, U &&y, const V &max_degree, const symbol_set &s)
{
    return detail::poly_mul_impl(::std::forward<T>(x), ::std::forward<U>(y), max_degree, s);
}

} // namespace polynomials

} // namespace piranha

#endif
