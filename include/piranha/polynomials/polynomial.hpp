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
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#include <piranha/byte_size.hpp>
#include <piranha/config.hpp>
#include <piranha/detail/hc.hpp>
#include <piranha/detail/ss_func_forward.hpp>
#include <piranha/detail/type_c.hpp>
#include <piranha/detail/xoroshiro128_plus.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/hash.hpp>
#include <piranha/key/key_merge_symbols.hpp>
#include <piranha/math/fma3.hpp>
#include <piranha/math/is_zero.hpp>
#include <piranha/math/safe_cast.hpp>
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
                // vector of copies of the original terms, or on vectors of const
                // pointers to the original terms.

                // The vectors of copies of input terms.
                // NOTE: need to drop the const on the key in order to enable sorting.
                using xv_t = ::std::vector<::std::pair<series_key_t<rT>, cf1_t>>;
                using yv_t = ::std::vector<::std::pair<series_key_t<rU>, cf2_t>>;

                // The vectors of pointers to the input terms.
                using xp_t = ::std::vector<const series_term_t<rT> *>;
                using yp_t = ::std::vector<const series_term_t<rU> *>;

                // The corresponding range types.
                // NOTE: in the code below, we always use cbegin()/cend() for the construction
                // of the ranges.
                using xr1_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<xv_t &>().cbegin(), poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<xv_t &>().cend(), poly_term_key_ref_extractor{})));
                using yr1_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<yv_t &>().cbegin(), poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<yv_t &>().cend(), poly_term_key_ref_extractor{})));

                using xr2_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<xp_t &>().cbegin(), poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<xp_t &>().cend(), poly_term_key_ref_extractor{})));
                using yr2_t = decltype(::piranha::detail::make_range(
                    ::boost::make_transform_iterator(::std::declval<yp_t &>().cbegin(), poly_term_key_ref_extractor{}),
                    ::boost::make_transform_iterator(::std::declval<yp_t &>().cend(), poly_term_key_ref_extractor{})));

                if constexpr (::std::conjunction_v<
                                  are_overflow_testable_monomial_ranges<xr1_t, yr1_t>,
                                  are_overflow_testable_monomial_ranges<xr2_t, yr2_t>,
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

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(push)
#pragma warning(disable : 4334)

#endif

// The multi-threaded homomorphic implementation.
template <typename Ret, typename T, typename U>
inline void poly_mul_impl_mt_hm(Ret &retval, const T &x, const U &y, unsigned log2_nsegs)
{
    using cf1_t = series_cf_t<T>;
    using cf2_t = series_cf_t<U>;
    using ret_key_t = series_key_t<Ret>;
    using ret_cf_t = series_cf_t<Ret>;

    // Preconditions.
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
    auto p_transform = [](const auto &p) { return ::std::make_pair(p.first, p.second); };
    ::std::vector<::std::pair<series_key_t<T>, cf1_t>> v1(::boost::make_transform_iterator(x.begin(), p_transform),
                                                          ::boost::make_transform_iterator(x.end(), p_transform));
    ::std::vector<::std::pair<series_key_t<U>, cf2_t>> v2(::boost::make_transform_iterator(y.begin(), p_transform),
                                                          ::boost::make_transform_iterator(y.end(), p_transform));

    // Do the monomial overflow checking.
    const bool overflow = !::piranha::monomial_range_overflow_check(
        ::piranha::detail::make_range(::boost::make_transform_iterator(v1.cbegin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(v1.cend(), poly_term_key_ref_extractor{})),
        ::piranha::detail::make_range(::boost::make_transform_iterator(v2.cbegin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(v2.cend(), poly_term_key_ref_extractor{})),
        ss);
    if (piranha_unlikely(overflow)) {
        piranha_throw(
            ::std::overflow_error,
            "An overflow in the monomial exponents was detected while attempting to multiply two polynomials");
    }

    // Sort the input terms according to the hash value modulo
    // 2**log2_nsegs. That is, sort them according to the bucket
    // they would occupy in a segmented table with 2**log2_nsegs
    // segments.
    auto t_sorter = [log2_nsegs](const auto &p1, const auto &p2) {
        const auto h1 = ::piranha::hash(p1.first);
        const auto h2 = ::piranha::hash(p2.first);

        return h1 % (1u << log2_nsegs) < h2 % (1u << log2_nsegs);
    };
    ::std::sort(v1.begin(), v1.end(), t_sorter);
    ::std::sort(v2.begin(), v2.end(), t_sorter);

    // Cache the number of segments.
    const auto nsegs = 1u << log2_nsegs;

    // Compute the segmentation for the input series.
    // The segmentation is a vector of ranges (represented
    // as pairs of iterators into v1/v2)
    // of size nsegs. The index at which each range is stored
    // is the index of the bucket that the terms corresponding
    // to that range would occupy in a segmented table
    // with 2**log2_nsegs segments.
    auto compute_vseg = [nsegs, log2_nsegs](auto &v) {
        using it_t = decltype(v.begin());

        ::std::vector<::std::pair<it_t, it_t>> vseg;
        vseg.resize(::piranha::safe_cast<decltype(vseg.size())>(nsegs));

        it_t it = v.begin();
        const auto v_end = v.end();
        for (auto i = 0u; i < nsegs; ++i) {
            vseg[i].first = it;
            it = ::std::upper_bound(it, v_end, i, [log2_nsegs](const auto &idx, const auto &p) {
                const auto h = ::piranha::hash(p.first);
                return idx < h % (1u << log2_nsegs);
            });
            vseg[i].second = it;
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

        auto verify_seg = [log2_nsegs](unsigned i, const auto &p, auto &counter) {
            for (auto it = p.first; it != p.second; ++it) {
                const auto h = ::piranha::hash(it->first);
                assert(h % (1u << log2_nsegs) == i);
                ++counter;
            }
        };

        for (auto i = 0u; i < nsegs; ++i) {
            verify_seg(i, vseg1[i], counter1);
            verify_seg(i, vseg2[i], counter2);
        }

        assert(counter1 == v1.size());
        assert(counter2 == v2.size());
    }
#endif

    // Setup the number of segments in retval.
    retval.set_n_segments(log2_nsegs);

#if !defined(NDEBUG)
    // Couple of debug variables that we use to
    // check that all term-by-term multiplications
    // are performed.
    ::std::atomic<unsigned long long> n_mults(0);
#endif

    // Wrap in try/catch to avoid assertion failures in
    // debug mode.
    try {
        // Parallel iteration over the number of buckets of the
        // output segmented table.
        ::tbb::parallel_for(::tbb::blocked_range(0u, nsegs), [&vseg1, &vseg2, nsegs, log2_nsegs, &retval, &ss
#if !defined(NDEBUG)
                                                              ,
                                                              &n_mults
#endif
        ](const auto &range) {
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
                // ranges vseg1[i] * vseg2[j] end up at the bucket
                // (i + j) % nsegs in retval. Thus, we need to select
                // all i, j pairs such that (i + j) % nsegs == seg_idx.
                for (auto i = 0u; i < nsegs; ++i) {
                    // NOTE: if seg_idx - i ends up negative, we wrap around
                    // and the modulo reduction does the right thing.
                    const auto j = (seg_idx - i) % (1u << log2_nsegs);
                    assert(j < vseg2.size());

                    // Fetch the corresponding ranges.
                    const auto r1 = vseg1[i];
                    const auto r2 = vseg2[j];

                    // The O(N**2) multiplication loop over the ranges.
                    for (auto it1 = r1.first; it1 != r1.second; ++it1) {
                        const auto &k1 = it1->first;
                        const auto &c1 = it1->second;

                        for (auto it2 = r2.first; it2 != r2.second; ++it2) {
                            const auto &c2 = it2->second;

                            // Do the monomial multiplication.
                            ::piranha::monomial_mul(tmp_key, k1, it2->first, ss);

                            // Check that the result ends up in the correct bucket.
                            assert(::piranha::hash(tmp_key) % (1u << log2_nsegs) == seg_idx);

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
            }
        });

#if !defined(NDEBUG)
        // Verify the number of term multiplications we performed.
        assert(n_mults.load() == static_cast<unsigned long long>(x.size()) * static_cast<unsigned long long>(y.size()));
#endif
    } catch (...) {
        // In case of exceptions, clear retval before
        // rethrowing to ensure a known sane state.
        retval.clear();
        throw;
    }
}

#if defined(_MSC_VER) && !defined(__clang__)

#pragma warning(pop)

#endif

// Simple poly mult implementation: just multiply
// term by term, no parallelisation, no segmentation,
// no copying of the operands, etc.
template <typename Ret, typename T, typename U>
inline void poly_mul_impl_simple(Ret &retval, const T &x, const U &y)
{
    using ret_key_t = series_key_t<Ret>;
    using ret_cf_t = series_cf_t<Ret>;
    using cf1_t = series_cf_t<T>;
    using cf2_t = series_cf_t<U>;

    // Preconditions.
    assert(retval.get_symbol_set() == x.get_symbol_set());
    assert(retval.get_symbol_set() == y.get_symbol_set());
    assert(retval.empty());
    assert(retval._get_s_table().size() == 1u);

    // Cache the symbol set.
    const auto &ss = retval.get_symbol_set();

    // Construct the vectors of pointer to the terms.
    auto ptr_extractor = [](const auto &p) { return &p; };
    ::std::vector<const series_term_t<T> *> v1(::boost::make_transform_iterator(x.begin(), ptr_extractor),
                                               ::boost::make_transform_iterator(x.end(), ptr_extractor));
    ::std::vector<const series_term_t<U> *> v2(::boost::make_transform_iterator(y.begin(), ptr_extractor),
                                               ::boost::make_transform_iterator(y.end(), ptr_extractor));

    // Do the monomial overflow checking.
    const bool overflow = !::piranha::monomial_range_overflow_check(
        ::piranha::detail::make_range(::boost::make_transform_iterator(v1.cbegin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(v1.cend(), poly_term_key_ref_extractor{})),
        ::piranha::detail::make_range(::boost::make_transform_iterator(v2.cbegin(), poly_term_key_ref_extractor{}),
                                      ::boost::make_transform_iterator(v2.cend(), poly_term_key_ref_extractor{})),
        ss);
    if (piranha_unlikely(overflow)) {
        piranha_throw(
            ::std::overflow_error,
            "An overflow in the monomial exponents was detected while attempting to multiply two polynomials");
    }

    // Proceed with the multiplication.
    auto &tab = retval._get_s_table()[0];

    // Wrap in try/catch to avoid assertion failures in
    // debug mode.
    try {
        // Temporary variable used in monomial multiplication.
        ret_key_t tmp_key;

        for (auto t1 : v1) {
            const auto &k1 = t1->first;
            const auto &c1 = t1->second;

            for (auto t2 : v2) {
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
    using ret_t = poly_mul_ret_t<T &&, U &&>;

    assert(x.get_symbol_set() == y.get_symbol_set());

    // Init the return value.
    ret_t retval;
    retval.set_symbol_set(x.get_symbol_set());

    // Fetch the hardware concurrency.
    // const auto hc = ::piranha::detail::hc();

    // if (hc == 1u) {
    //     // We have only a single core available,
    //     // run the simple implementation.
    //     detail::poly_mul_impl_simple(retval, x, y);
    // } else {
    //     using int_t = ::mppp::integer<1>;

    //     // TODO: proper heuristic for mt mode.
    //     // TODO: ensure hc cannot possibly be zero.
    //     // For now, hard code to circa number of cores * 2.
    //     const auto log2_nthreads
    //         = ::std::min(::piranha::safe_cast<unsigned>(int_t{hc}.nbits()), retval._get_max_log2_size());

    // NOTE: also need to check for homomorphicity.
    // NOTE: ensure that log2_nsegs is within the
    // retval._get_max_log2_size(), which ensures we can
    // always use it as a shift argument for unsigned.

    // detail::poly_mul_impl_mt(retval, x, y, 11);
    detail::poly_mul_impl_simple(retval, x, y);
    // }

    return retval;
}

} // namespace detail

template <typename T, typename U, ::std::enable_if_t<detail::poly_mul_algo<T &&, U &&> != 0, int> = 0>
inline detail::poly_mul_ret_t<T &&, U &&> series_mul(T &&x, U &&y)
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

} // namespace piranha

#endif
