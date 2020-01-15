// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)::polynomials
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POLYNOMIALS_PACKED_MONOMIAL_HPP
#define OBAKE_POLYNOMIALS_PACKED_MONOMIAL_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/serialization/access.hpp>

#include <tbb/blocked_range.h>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_reduce.h>

#include <mp++/integer.hpp>

#include <obake/config.hpp>
#include <obake/detail/ignore.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/mppp_utils.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/type_c.hpp>
#include <obake/exceptions.hpp>
#include <obake/k_packing.hpp>
#include <obake/math/pow.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/math/safe_convert.hpp>
#include <obake/polynomials/monomial_homomorphic_hash.hpp>
#include <obake/ranges.hpp>
#include <obake/s11n.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace polynomials
{

#if defined(OBAKE_HAVE_CONCEPTS)
template <KPackable T>
#else
template <typename T, typename = ::std::enable_if_t<is_k_packable_v<T>>>
#endif
class packed_monomial
{
    friend class ::boost::serialization::access;

public:
    // Alias for T.
    using value_type = T;

    // Def ctor inits to a monomial with all zero exponents.
    constexpr packed_monomial() : m_value(0) {}
    // Constructor from symbol set.
    constexpr explicit packed_monomial(const symbol_set &) : packed_monomial() {}
    // Constructor from value.
    constexpr explicit packed_monomial(const T &n) : m_value(n) {}
    // Constructor from input iterator and size.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <typename It>
    requires InputIterator<It> &&SafelyCastable<typename ::std::iterator_traits<It>::reference, T>
#else
    template <
        typename It,
        ::std::enable_if_t<::std::conjunction_v<is_input_iterator<It>,
                                                is_safely_castable<typename ::std::iterator_traits<It>::reference, T>>,
                           int> = 0>
#endif
        constexpr explicit packed_monomial(It it, unsigned n)
    {
        k_packer<T> kp(n);
        for (auto i = 0u; i < n; ++i, ++it) {
            kp << ::obake::safe_cast<T>(*it);
        }
        m_value = kp.get();
    }

private:
    struct fwd_it_ctor_tag {
    };
    template <typename It>
    constexpr explicit packed_monomial(fwd_it_ctor_tag, It b, It e)
    {
        k_packer<T> kp(::obake::safe_cast<unsigned>(::std::distance(b, e)));
        for (; b != e; ++b) {
            kp << ::obake::safe_cast<T>(*b);
        }
        m_value = kp.get();
    }

public:
    // Ctor from a pair of forward iterators.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <typename It>
    requires ForwardIterator<It> &&SafelyCastable<typename ::std::iterator_traits<It>::difference_type, unsigned> &&
        SafelyCastable<typename ::std::iterator_traits<It>::reference, T>
#else
    template <typename It,
              ::std::enable_if_t<::std::conjunction_v<
                                     is_forward_iterator<It>,
                                     is_safely_castable<typename ::std::iterator_traits<It>::difference_type, unsigned>,
                                     is_safely_castable<typename ::std::iterator_traits<It>::reference, T>>,
                                 int> = 0>
#endif
        constexpr explicit packed_monomial(It b, It e) : packed_monomial(fwd_it_ctor_tag{}, b, e)
    {
    }
    // Ctor from forward range.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <typename Range>
    requires ForwardRange<Range> &&
        SafelyCastable<typename ::std::iterator_traits<range_begin_t<Range>>::difference_type, unsigned> &&
            SafelyCastable<typename ::std::iterator_traits<range_begin_t<Range>>::reference, T>
#else
    template <
        typename Range,
        ::std::enable_if_t<
            ::std::conjunction_v<
                is_forward_range<Range>,
                is_safely_castable<typename ::std::iterator_traits<range_begin_t<Range>>::difference_type, unsigned>,
                is_safely_castable<typename ::std::iterator_traits<range_begin_t<Range>>::reference, T>>,
            int> = 0>
#endif
        constexpr explicit packed_monomial(Range &&r)
        : packed_monomial(fwd_it_ctor_tag{}, ::obake::begin(::std::forward<Range>(r)),
                          ::obake::end(::std::forward<Range>(r)))
    {
    }
    // Ctor from init list.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <typename U>
    requires SafelyCastable<const U &, T>
#else
    template <typename U, ::std::enable_if_t<is_safely_castable_v<const U &, T>, int> = 0>
#endif
        constexpr explicit packed_monomial(::std::initializer_list<U> l)
        : packed_monomial(fwd_it_ctor_tag{}, l.begin(), l.end())
    {
    }
    // Getter for the internal value.
    constexpr const T &get_value() const
    {
        return m_value;
    }
    // Setter for the internal value.
    constexpr void _set_value(const T &n)
    {
        m_value = n;
    }

private:
    // Serialisation.
    template <class Archive>
    void serialize(Archive &ar, unsigned)
    {
        ar &m_value;
    }

private:
    T m_value;
};

// Implementation of key_is_zero(). A monomial is never zero.
template <typename T>
constexpr bool key_is_zero(const packed_monomial<T> &, const symbol_set &)
{
    return false;
}

// Implementation of key_is_one(). A monomial is one if all its exponents are zero.
template <typename T>
constexpr bool key_is_one(const packed_monomial<T> &p, const symbol_set &)
{
    return p.get_value() == T(0);
}

// Comparison operators.
template <typename T>
constexpr bool operator==(const packed_monomial<T> &m1, const packed_monomial<T> &m2)
{
    return m1.get_value() == m2.get_value();
}

template <typename T>
constexpr bool operator!=(const packed_monomial<T> &m1, const packed_monomial<T> &m2)
{
    return m1.get_value() != m2.get_value();
}

// Hash implementation.
template <typename T>
constexpr ::std::size_t hash(const packed_monomial<T> &m)
{
    return static_cast<::std::size_t>(m.get_value());
}

// Symbol set compatibility implementation.
template <typename T>
inline bool key_is_compatible(const packed_monomial<T> &m, const symbol_set &s)
{
    const auto s_size = s.size();

    if (s_size == 0u) {
        // In case of an empty symbol set,
        // the only valid value for the monomial
        // is zero.
        return m.get_value() == T(0);
    }

    if (s_size == 1u) {
        // For unitary packing, all possible
        // values for T are allowed.
        return true;
    }

    // For non-unitary packing, we have to check that:
    // - the size of the symbol set is not too large,
    // - the current encoded value is within the limits.
    if (s_size > ::obake::detail::k_packing_get_max_size<T>()) {
        return false;
    }

    // The size of the symbol set is at least 2 and within the
    // limits. Check the encoded value.
    // NOTE: static cast is fine, s_size is within the limits.
    const auto &e_lim = ::obake::detail::k_packing_get_elimits<T>(static_cast<unsigned>(s_size));
    if constexpr (is_signed_v<T>) {
        return m.get_value() >= e_lim[0] && m.get_value() <= e_lim[1];
    } else {
        return m.get_value() <= e_lim;
    }
}

// Implementation of stream insertion.
// NOTE: requires that m is compatible with s.
template <typename T>
inline void key_stream_insert(::std::ostream &os, const packed_monomial<T> &m, const symbol_set &s)
{
    assert(polynomials::key_is_compatible(m, s));

    // NOTE: we know s is not too large from the assert.
    const auto s_size = static_cast<unsigned>(s.size());
    bool wrote_something = false;
    k_unpacker<T> ku(m.get_value(), s_size);
    T tmp;

    for (const auto &var : s) {
        ku >> tmp;
        if (tmp != T(0)) {
            // The exponent of the current variable
            // is nonzero.
            if (wrote_something) {
                // We already printed something
                // earlier, make sure we put
                // the multiplication sign
                // in front of the variable
                // name.
                os << '*';
            }
            // Print the variable name.
            os << var;
            wrote_something = true;
            if (tmp != T(1)) {
                // The exponent is not unitary,
                // print it.
                os << "**" << ::obake::detail::to_string(tmp);
            }
        }
    }

    if (!wrote_something) {
        // We did not write anything to the stream.
        // It means that all variables have zero
        // exponent, thus we print only "1".
        assert(m.get_value() == T(0));
        os << '1';
    }
}

// Implementation of tex stream insertion.
// NOTE: requires that m is compatible with s.
template <typename T>
inline void key_tex_stream_insert(::std::ostream &os, const packed_monomial<T> &m, const symbol_set &s)
{
    assert(polynomials::key_is_compatible(m, s));

    // Use separate streams for numerator and denominator
    // (the denominator is used only in case of negative powers).
    ::std::ostringstream oss_num, oss_den, *cur_oss;

    // NOTE: we know s is not too large from the assert.
    const auto s_size = static_cast<unsigned>(s.size());
    k_unpacker<T> ku(m.get_value(), s_size);
    T tmp;
    // Go through a multiprecision integer for the stream
    // insertion. This allows us not to care about potential
    // overflow conditions when manipulating the exponents
    // below.
    ::mppp::integer<1> tmp_mp;
    for (const auto &var : s) {
        // Extract the current exponent into
        // tmp_mp.
        ku >> tmp;
        tmp_mp = tmp;

        const auto sgn = tmp_mp.sgn();
        if (sgn != 0) {
            // Non-zero exponent, we will write something.
            if (sgn == 1) {
                // Positive exponent, we will write
                // to the numerator stream.
                cur_oss = &oss_num;
            } else {
                // Negative exponent: take the absolute value
                // and write to the denominator stream.
                tmp_mp.neg();
                cur_oss = &oss_den;
            }

            // Print the symbol name.
            *cur_oss << '{' << var << '}';

            // Raise to power, if the exponent is not one.
            if (!tmp_mp.is_one()) {
                *cur_oss << "^{" << tmp_mp << '}';
            }
        }
    }

    const auto num_str = oss_num.str(), den_str = oss_den.str();

    if (!num_str.empty() && !den_str.empty()) {
        // We have both negative and positive exponents,
        // print them both in a fraction.
        os << "\\frac{" << num_str << "}{" << den_str << '}';
    } else if (!num_str.empty() && den_str.empty()) {
        // Only positive exponents.
        os << num_str;
    } else if (num_str.empty() && !den_str.empty()) {
        // Only negative exponents, display them as 1/something.
        os << "\\frac{1}{" << den_str << '}';
    }
}

// Implementation of symbols merging.
// NOTE: requires that m is compatible with s, and ins_map consistent with s.
template <typename T>
inline packed_monomial<T> key_merge_symbols(const packed_monomial<T> &m, const symbol_idx_map<symbol_set> &ins_map,
                                            const symbol_set &s)
{
    assert(polynomials::key_is_compatible(m, s));
    // The last element of the insertion map must be at most s.size(), which means that there
    // are symbols to be appended at the end.
    assert(ins_map.empty() || ins_map.rbegin()->first <= s.size());

    // Do a first pass to compute the total size after merging.
    auto merged_size = s.size();
    for (const auto &p : ins_map) {
        const auto tmp_size = p.second.size();
        // LCOV_EXCL_START
        if (obake_unlikely(tmp_size > ::obake::detail::limits_max<decltype(s.size())> - merged_size)) {
            obake_throw(::std::overflow_error, "Overflow while trying to merge new symbols in a packed monomial: the "
                                               "size of the merged monomial is too large");
        }
        // LCOV_EXCL_STOP
        merged_size += tmp_size;
    }

    // Init the unpacker and the packer.
    // NOTE: we know s.size() is small enough thanks to the
    // assertion at the beginning.
    k_unpacker<T> ku(m.get_value(), static_cast<unsigned>(s.size()));
    k_packer<T> kp(::obake::safe_cast<unsigned>(merged_size));

    auto map_it = ins_map.begin();
    const auto map_end = ins_map.end();
    for (auto i = 0u; i < static_cast<unsigned>(s.size()); ++i) {
        if (map_it != map_end && map_it->first == i) {
            // We reached an index at which we need to
            // insert new elements. Insert as many
            // zeroes as necessary in the packer.
            for (const auto &_ : map_it->second) {
                ::obake::detail::ignore(_);
                kp << T(0);
            }
            // Move to the next element in the map.
            ++map_it;
        }
        // Add the existing element to the packer.
        T tmp;
        ku >> tmp;
        kp << tmp;
    }

    // We could still have symbols which need to be appended at the end.
    if (map_it != map_end) {
        for (const auto &_ : map_it->second) {
            ::obake::detail::ignore(_);
            kp << T(0);
        }
        assert(map_it + 1 == map_end);
    }

    return packed_monomial<T>(kp.get());
}

// Implementation of monomial_mul().
// NOTE: requires a, b and out to be compatible with ss.
template <typename T>
constexpr void monomial_mul(packed_monomial<T> &out, const packed_monomial<T> &a, const packed_monomial<T> &b,
                            [[maybe_unused]] const symbol_set &ss)
{
    // Verify the inputs.
    assert(polynomials::key_is_compatible(a, ss));
    assert(polynomials::key_is_compatible(b, ss));
    assert(polynomials::key_is_compatible(out, ss));

    out._set_value(a.get_value() + b.get_value());

    // Verify the output as well.
    assert(polynomials::key_is_compatible(out, ss));
}

namespace detail
{

// Small helper to detect if 2 types
// are the same packed_monomial type.
template <typename, typename>
struct same_packed_monomial : ::std::false_type {
};

template <typename T>
struct same_packed_monomial<packed_monomial<T>, packed_monomial<T>> : ::std::true_type {
};

template <typename T, typename U>
inline constexpr bool same_packed_monomial_v = same_packed_monomial<T, U>::value;

} // namespace detail

// Monomial overflow checking.
// NOTE: this assumes that all the monomials in the 2 ranges
// are compatible with ss.
// NOTE: this can be parallelised. We need:
// - a good heuristic (should not be too difficult given
//   the constraints on packed_monomial),
// - the random-access iterator concept.
#if defined(OBAKE_HAVE_CONCEPTS)
template <typename R1, typename R2>
requires InputRange<R1> &&InputRange<R2> &&
    detail::same_packed_monomial_v<remove_cvref_t<typename ::std::iterator_traits<range_begin_t<R1>>::reference>,
                                   remove_cvref_t<typename ::std::iterator_traits<range_begin_t<R2>>::reference>>
#else
template <typename R1, typename R2,
          ::std::enable_if_t<
              ::std::conjunction_v<is_input_range<R1>, is_input_range<R2>,
                                   detail::same_packed_monomial<
                                       remove_cvref_t<typename ::std::iterator_traits<range_begin_t<R1>>::reference>,
                                       remove_cvref_t<typename ::std::iterator_traits<range_begin_t<R2>>::reference>>>,
              int> = 0>
#endif
    inline bool monomial_range_overflow_check(R1 &&r1, R2 &&r2, const symbol_set &ss)
{
    using pm_t = remove_cvref_t<typename ::std::iterator_traits<range_begin_t<R1>>::reference>;
    using value_type = typename pm_t::value_type;
    using int_t = ::mppp::integer<1>;

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    if (s_size == 0u) {
        // If the monomials have zero variables,
        // there cannot be overflow.
        return true;
    }

    // Get out the begin/end iterators.
    auto b1 = ::obake::begin(::std::forward<R1>(r1));
    const auto e1 = ::obake::end(::std::forward<R1>(r1));
    auto b2 = ::obake::begin(::std::forward<R2>(r2));
    const auto e2 = ::obake::end(::std::forward<R2>(r2));

    if (b1 == e1 || b2 == e2) {
        // If either range is empty, there will be no overflow.
        return true;
    }

    // Prepare the limits vectors.
    auto [limits1, limits2] = [s_size]() {
        if constexpr (is_signed_v<value_type>) {
            ::std::vector<::std::pair<value_type, value_type>> minmax1, minmax2;
            minmax1.reserve(static_cast<decltype(minmax1.size())>(s_size));
            minmax2.reserve(static_cast<decltype(minmax2.size())>(s_size));

            return ::std::make_tuple(::std::move(minmax1), ::std::move(minmax2));
        } else {
            ::std::vector<value_type> max1, max2;
            max1.reserve(static_cast<decltype(max1.size())>(s_size));
            max2.reserve(static_cast<decltype(max2.size())>(s_size));

            return ::std::make_tuple(::std::move(max1), ::std::move(max2));
        }
    }();

    // Init the limits vectors with the first elements of the ranges.
    {
        // NOTE: if the iterators return copies
        // of the monomials, rather than references,
        // capturing them via const
        // ref will extend their lifetimes.
        const auto &init1 = *b1;
        const auto &init2 = *b2;

        assert(polynomials::key_is_compatible(init1, ss));
        assert(polynomials::key_is_compatible(init2, ss));

        k_unpacker<value_type> ku1(init1.get_value(), s_size);
        k_unpacker<value_type> ku2(init2.get_value(), s_size);
        value_type tmp;
        for (auto i = 0u; i < s_size; ++i) {
            ku1 >> tmp;
            if constexpr (is_signed_v<value_type>) {
                limits1.emplace_back(tmp, tmp);
            } else {
                limits1.emplace_back(tmp);
            }
            ku2 >> tmp;
            if constexpr (is_signed_v<value_type>) {
                limits2.emplace_back(tmp, tmp);
            } else {
                limits2.emplace_back(tmp);
            }
        }
    }

    // Serial implementation.
    auto serial_impl = [&ss, s_size, b1, e1, &l1 = limits1, b2, e2, &l2 = limits2]() {
        // Helper to examine the rest of the ranges.
        auto update_minmax = [&ss, s_size](auto b, auto e, auto &limits) {
            ::obake::detail::ignore(ss);

            for (++b; b != e; ++b) {
                const auto &cur = *b;

                assert(polynomials::key_is_compatible(cur, ss));

                k_unpacker<value_type> ku(cur.get_value(), s_size);
                value_type tmp;
                for (auto i = 0u; i < s_size; ++i) {
                    ku >> tmp;
                    if constexpr (is_signed_v<value_type>) {
                        limits[i].first = ::std::min(limits[i].first, tmp);
                        limits[i].second = ::std::max(limits[i].second, tmp);
                    } else {
                        limits[i] = ::std::max(limits[i], tmp);
                    }
                }
            }
        };
        // Examine the rest of the ranges.
        update_minmax(b1, e1, l1);
        update_minmax(b2, e2, l2);
    };

    if constexpr (::std::conjunction_v<is_random_access_iterator<decltype(b1)>,
                                       is_random_access_iterator<decltype(b2)>>) {
        // If both ranges are random-access, we have the option of running a parallel
        // overflow check.
        const auto size1 = ::std::distance(b1, e1);
        const auto size2 = ::std::distance(b2, e2);

        // NOTE: run the parallel implementation only if
        // at least one of the sizes is large enough.
        if (size1 > 5000 || size2 > 5000) {
            auto par_functor = [&ss, s_size](auto b, auto e, const auto &limits) {
                return ::tbb::parallel_reduce(
                    // NOTE: the ranges are guaranteed to be non-empty,
                    // thus b + 1 is always well-defined.
                    ::tbb::blocked_range<decltype(b)>(b + 1, e), limits,
                    [&ss, s_size](const auto &range, auto cur) {
                        ::obake::detail::ignore(ss);

                        for (const auto &m : range) {
                            assert(polynomials::key_is_compatible(m, ss));

                            k_unpacker<value_type> ku(m.get_value(), s_size);
                            value_type tmp;
                            for (auto i = 0u; i < s_size; ++i) {
                                ku >> tmp;
                                if constexpr (is_signed_v<value_type>) {
                                    cur[i].first = ::std::min(cur[i].first, tmp);
                                    cur[i].second = ::std::max(cur[i].second, tmp);
                                } else {
                                    cur[i] = ::std::max(cur[i], tmp);
                                }
                            }
                        }

                        return cur;
                    },
                    [s_size](const auto &l1, const auto &l2) {
                        assert(l1.size() == s_size);
                        assert(l2.size() == s_size);

                        remove_cvref_t<decltype(l1)> ret;
                        ret.reserve(s_size);

                        for (auto i = 0u; i < s_size; ++i) {
                            if constexpr (is_signed_v<value_type>) {
                                ret.emplace_back(::std::min(l1[i].first, l2[i].first),
                                                 ::std::max(l1[i].second, l2[i].second));
                            } else {
                                ret.emplace_back(::std::max(l1[i], l2[i]));
                            }
                        }

                        return ret;
                    });
            };

            ::tbb::parallel_invoke([par_functor, b1, e1, &l1 = limits1]() { l1 = par_functor(b1, e1, l1); },
                                   [par_functor, b2, e2, &l2 = limits2]() { l2 = par_functor(b2, e2, l2); });
        } else {
            serial_impl();
        }
    } else {
        serial_impl();
    }

    // Now add the limits via interval arithmetics
    // and check for overflow. Use mppp::integer for the check.

    // Fetch the delta bit width from the size.
    const auto nbits = ::obake::detail::k_packing_size_to_bits<value_type>(s_size);

    if constexpr (is_signed_v<value_type>) {
        for (decltype(limits1.size()) i = 0; i < s_size; ++i) {
            const auto add_min = int_t{limits1[i].first} + limits2[i].first;
            const auto add_max = int_t{limits1[i].second} + limits2[i].second;

            // NOTE: need to special-case s_size == 1, in which case
            // the component limits are the full numerical range of the type.
            const auto lim_min
                = s_size == 1u ? ::obake::detail::limits_min<value_type>
                               : ::obake::detail::k_packing_get_climits<value_type>(nbits, static_cast<unsigned>(i))[0];
            const auto lim_max
                = s_size == 1u ? ::obake::detail::limits_max<value_type>
                               : ::obake::detail::k_packing_get_climits<value_type>(nbits, static_cast<unsigned>(i))[1];

            // NOTE: an overflow condition will likely result in an exception
            // or some other error handling. Optimise for the non-overflow case.
            if (obake_unlikely(add_min < lim_min || add_max > lim_max)) {
                return false;
            }
        }
    } else {
        for (decltype(limits1.size()) i = 0; i < s_size; ++i) {
            const auto add_max = int_t{limits1[i]} + limits2[i];

            // NOTE: like above, special-case s_size == 1.
            const auto lim_max
                = s_size == 1u ? ::obake::detail::limits_max<value_type>
                               : ::obake::detail::k_packing_get_climits<value_type>(nbits, static_cast<unsigned>(i));

            if (obake_unlikely(add_max > lim_max)) {
                return false;
            }
        }
    }

    return true;
}

// Implementation of key_degree().
// NOTE: this assumes that p is compatible with ss.
template <typename T>
inline T key_degree(const packed_monomial<T> &p, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    T retval(0), tmp;
    k_unpacker<T> ku(p.get_value(), s_size);
    for (auto i = 0u; i < s_size; ++i) {
        ku >> tmp;
        retval += tmp;
    }

    return retval;
}

// Implementation of key_p_degree().
// NOTE: this assumes that p and si are compatible with ss.
template <typename T>
inline T key_p_degree(const packed_monomial<T> &p, const symbol_idx_set &si, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    assert(si.empty() || *(si.end() - 1) < ss.size());

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    T retval(0), tmp;
    k_unpacker<T> ku(p.get_value(), s_size);
    auto si_it = si.begin();
    const auto si_it_end = si.end();
    for (auto i = 0u; i < s_size && si_it != si_it_end; ++i) {
        ku >> tmp;
        if (i == *si_it) {
            retval += tmp;
            ++si_it;
        }
    }

    assert(si_it == si_it_end);

    return retval;
}

// Monomial exponentiation.
// NOTE: this assumes that p is compatible with ss.
template <typename T, typename U,
          ::std::enable_if_t<::std::disjunction_v<::obake::detail::is_mppp_integer<U>,
                                                  is_safely_convertible<const U &, ::mppp::integer<1> &>>,
                             int> = 0>
inline packed_monomial<T> monomial_pow(const packed_monomial<T> &p, const U &n, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    // NOTE: exp will be a const ref if n is already
    // an mppp integer, a new value otherwise.
    decltype(auto) exp = [&n]() -> decltype(auto) {
        if constexpr (::obake::detail::is_mppp_integer_v<U>) {
            return n;
        } else {
            ::mppp::integer<1> ret;

            if (obake_unlikely(!::obake::safe_convert(ret, n))) {
                if constexpr (is_stream_insertable_v<const U &>) {
                    // Provide better error message if U is ostreamable.
                    ::std::ostringstream oss;
                    static_cast<::std::ostream &>(oss) << n;
                    obake_throw(::std::invalid_argument, "Invalid exponent for monomial exponentiation: the exponent ("
                                                             + oss.str()
                                                             + ") cannot be converted into an integral value");
                } else {
                    obake_throw(::std::invalid_argument, "Invalid exponent for monomial exponentiation: the exponent "
                                                         "cannot be converted into an integral value");
                }
            }

            return ret;
        }
    }();

    // Unpack, multiply in arbitrary-precision arithmetic, re-pack.
    k_unpacker<T> ku(p.get_value(), s_size);
    k_packer<T> kp(s_size);
    T tmp;
    remove_cvref_t<decltype(exp)> tmp_int;
    for (auto i = 0u; i < s_size; ++i) {
        ku >> tmp;
        tmp_int = tmp;
        tmp_int *= exp;
        kp << static_cast<T>(tmp_int);
    }

    return packed_monomial<T>(kp.get());
}

namespace detail
{

// Metaprogramming for the key_evaluate() implementation for packed_monomial.
template <typename T, typename U>
constexpr auto pm_key_evaluate_algorithm_impl()
{
    [[maybe_unused]] constexpr auto failure = ::std::make_pair(0, ::obake::detail::type_c<void>{});

    // Test exponentiability via const lvalue refs.
    if constexpr (is_exponentiable_v<::std::add_lvalue_reference_t<const U>, ::std::add_lvalue_reference_t<const T>>) {
        using ret_t = ::obake::detail::pow_t<const U &, const T &>;

        // We will need to construct the return value from 1,
        // multiply it in-place and then return it.
        // NOTE: require also a semi-regular type,
        // it's just easier to reason about.
        if constexpr (::std::conjunction_v<::std::is_constructible<ret_t, int>,
                                           // NOTE: we will be multiplying an
                                           // lvalue by an rvalue.
                                           is_in_place_multipliable<::std::add_lvalue_reference_t<ret_t>, ret_t>,
                                           is_semi_regular<ret_t>, is_returnable<ret_t>>) {
            return ::std::make_pair(1, ::obake::detail::type_c<ret_t>{});
        } else {
            return failure;
        }
    } else {
        return failure;
    }
}

// Shortcuts.
template <typename T, typename U>
inline constexpr auto pm_key_evaluate_algorithm = detail::pm_key_evaluate_algorithm_impl<T, U>();

template <typename T, typename U>
inline constexpr int pm_key_evaluate_algo = pm_key_evaluate_algorithm<T, U>.first;

template <typename T, typename U>
using pm_key_evaluate_ret_t = typename decltype(pm_key_evaluate_algorithm<T, U>.second)::type;

} // namespace detail

// Evaluation of a packed monomial.
// NOTE: this requires that p is compatible with ss,
// and that sm is consistent with ss.
template <typename T, typename U, ::std::enable_if_t<detail::pm_key_evaluate_algo<T, U> != 0, int> = 0>
inline detail::pm_key_evaluate_ret_t<T, U> key_evaluate(const packed_monomial<T> &p, const symbol_idx_map<U> &sm,
                                                        const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    // sm and ss must have the same size, and the last element
    // of sm must have an index equal to the last index of ss.
    assert(sm.size() == ss.size() && (sm.empty() || (sm.cend() - 1)->first == ss.size() - 1u));

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    // Init the return value and the unpacking machinery.
    detail::pm_key_evaluate_ret_t<T, U> retval(1);
    k_unpacker<T> ku(p.get_value(), s_size);
    T tmp;
    // Accumulate the result.
    for (const auto &pr : sm) {
        ku >> tmp;
        retval *= ::obake::pow(pr.second, ::std::as_const(tmp));
    }

    return retval;
}

namespace detail
{

// NOTE: the metaprogramming for the monomial_subs() implementation for packed_monomial
// is identical to the key_evaluate() implementation.

// Shortcuts.
template <typename T, typename U>
inline constexpr auto pm_monomial_subs_algorithm = detail::pm_key_evaluate_algorithm_impl<T, U>();

template <typename T, typename U>
inline constexpr int pm_monomial_subs_algo = pm_key_evaluate_algorithm<T, U>.first;

template <typename T, typename U>
using pm_monomial_subs_ret_t = typename decltype(pm_key_evaluate_algorithm<T, U>.second)::type;

} // namespace detail

// Substitution of symbols in a packed monomial.
// NOTE: this requires that p is compatible with ss,
// and that sm is consistent with ss.
template <typename T, typename U, ::std::enable_if_t<detail::pm_monomial_subs_algo<T, U> != 0, int> = 0>
inline ::std::pair<detail::pm_monomial_subs_ret_t<T, U>, packed_monomial<T>>
monomial_subs(const packed_monomial<T> &p, const symbol_idx_map<U> &sm, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    // sm must not be larger than ss, and the last element
    // of sm must have an index smaller than the size of ss.
    assert(sm.size() <= ss.size() && (sm.empty() || (sm.cend() - 1)->first < ss.size()));

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    // Init the return value and the (un)packing machinery.
    detail::pm_monomial_subs_ret_t<T, U> retval(1);
    k_unpacker<T> ku(p.get_value(), s_size);
    k_packer<T> kp(s_size);
    T tmp;
    auto sm_it = sm.cbegin();
    const auto sm_end = sm.cend();
    for (auto i = 0u; i < s_size; ++i) {
        ku >> tmp;

        if (sm_it != sm_end && sm_it->first == i) {
            // The current exponent is in the subs map,
            // accumulate the result of the substitution.
            retval *= ::obake::pow(sm_it->second, ::std::as_const(tmp));

            // Set the exponent to zero in the output
            // monomial.
            kp << T(0);

            // Move to the next item in the map.
            ++sm_it;
        } else {
            // Either the current exponent is not in the subs map,
            // or we already reached the end of the map.
            // Just copy the original exponent into the output monomial.
            kp << tmp;
        }
    }
    assert(sm_it == sm_end);

    return ::std::make_pair(::std::move(retval), packed_monomial<T>(kp.get()));
}

// Identify non-trimmable exponents in p.
// NOTE: this requires that p is compatible with ss,
// and that v has the same size as ss.
template <typename T>
inline void key_trim_identify(::std::vector<int> &v, const packed_monomial<T> &p, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    assert(v.size() == ss.size());

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    k_unpacker<T> ku(p.get_value(), s_size);
    T tmp;
    for (auto i = 0u; i < s_size; ++i) {
        ku >> tmp;

        if (tmp != T(0)) {
            // The current exponent is nonzero,
            // thus it must not be trimmed.
            v[i] = 0;
        }
    }
}

// Eliminate from p the exponents at the indices
// specifed by si.
// NOTE: this requires that p is compatible with ss,
// and that si is consistent with ss.
template <typename T>
inline packed_monomial<T> key_trim(const packed_monomial<T> &p, const symbol_idx_set &si, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    // NOTE: si cannot be larger than ss, and its last element must be smaller
    // than the size of ss.
    assert(si.size() <= ss.size() && (si.empty() || *(si.cend() - 1) < ss.size()));

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    k_unpacker<T> ku(p.get_value(), s_size);
    k_packer<T> kp(static_cast<unsigned>(s_size - si.size()));
    T tmp;
    auto si_it = si.cbegin();
    const auto si_end = si.cend();
    for (auto i = 0u; i < s_size; ++i) {
        ku >> tmp;

        if (si_it != si_end && *si_it == i) {
            // The current exponent must be trimmed. Don't
            // add it to kp, and move to the next item in the trim set.
            ++si_it;
        } else {
            // The current exponent must be kept.
            kp << tmp;
        }
    }
    assert(si_it == si_end);

    return packed_monomial<T>(kp.get());
}

// Monomial differentiation.
// NOTE: this requires that p is compatible with ss,
// and idx is within ss.
template <typename T>
inline ::std::pair<T, packed_monomial<T>> monomial_diff(const packed_monomial<T> &p, const symbol_idx &idx,
                                                        const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    assert(idx < ss.size());

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    // Init the (un)packing machinery.
    k_unpacker<T> ku(p.get_value(), s_size);
    k_packer<T> kp(s_size);
    T tmp, ret_exp(0);
    for (auto i = 0u; i < s_size; ++i) {
        ku >> tmp;

        if (i == idx && tmp != T(0)) {
            // NOTE: the exponent of the differentiation variable
            // is not zero. Take the derivative.
            // NOTE: if the exponent is zero, ret_exp will remain to
            // its initial value (0) and the output monomial
            // will be the same as p.
            if (obake_unlikely(tmp == ::obake::detail::limits_min<T>)) {
                obake_throw(::std::overflow_error,
                            "Overflow detected while computing the derivative of a packed monomial: the exponent of "
                            "the variable with respect to which the differentiation is being taken ('"
                                + *ss.nth(static_cast<decltype(ss.size())>(i)) + "') is too small ("
                                + ::obake::detail::to_string(tmp)
                                + "), and taking the derivative would generate a negative overflow");
            }
            ret_exp = tmp--;
        }

        kp << tmp;
    }

    return ::std::make_pair(ret_exp, packed_monomial<T>(kp.get()));
}

// Monomial integration.
// NOTE: this requires that p is compatible with ss,
// and idx is within ss.
template <typename T>
inline ::std::pair<T, packed_monomial<T>> monomial_integrate(const packed_monomial<T> &p, const symbol_idx &idx,
                                                             const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    assert(idx < ss.size());

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    // Init the (un)packing machinery.
    k_unpacker<T> ku(p.get_value(), s_size);
    k_packer<T> kp(s_size);
    T tmp, ret_exp(0);
    for (auto i = 0u; i < s_size; ++i) {
        ku >> tmp;

        if (i == idx) {
            if constexpr (is_signed_v<T>) {
                // For signed integrals, make sure
                // we are not integrating x**-1.
                if (obake_unlikely(tmp == T(-1))) {
                    obake_throw(::std::domain_error,
                                "Cannot integrate a packed monomial: the exponent of the integration variable ('"
                                    + *ss.nth(static_cast<decltype(ss.size())>(i))
                                    + "') is -1, and the integration would generate a logarithmic term");
                }
            }

            if (obake_unlikely(tmp == ::obake::detail::limits_max<T>)) {
                obake_throw(::std::overflow_error,
                            "Overflow detected while computing the integral of a packed monomial: the exponent of "
                            "the integration variable ('"
                                + *ss.nth(static_cast<decltype(ss.size())>(i)) + "') is too large ("
                                + ::obake::detail::to_string(tmp)
                                + "), and the computation would generate a positive overflow");
            }
            ret_exp = ++tmp;
        }

        kp << tmp;
    }
    // We must have written some nonzero value to ret_exp.
    assert(ret_exp != T(0));

    return ::std::make_pair(ret_exp, packed_monomial<T>(kp.get()));
}

} // namespace polynomials

// Lift to the obake namespace.
template <typename T>
using packed_monomial = polynomials::packed_monomial<T>;

// Specialise monomial_has_homomorphic_hash.
template <typename T>
inline constexpr bool monomial_hash_is_homomorphic<packed_monomial<T>> = true;

} // namespace obake

namespace boost::serialization
{

// Disable tracking for packed_monomial.
template <typename T>
struct tracking_level<::obake::packed_monomial<T>> : ::obake::detail::s11n_no_tracking<::obake::packed_monomial<T>> {
};

} // namespace boost::serialization

#endif
