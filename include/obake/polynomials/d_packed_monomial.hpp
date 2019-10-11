// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)::polynomials
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POLYNOMIALS_D_PACKED_MONOMIAL_HPP
#define OBAKE_POLYNOMIALS_D_PACKED_MONOMIAL_HPP

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

#include <boost/container/small_vector.hpp>

#include <mp++/integer.hpp>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/k_packing.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/polynomials/monomial_homomorphic_hash.hpp>
#include <obake/ranges.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace polynomials
{

namespace detail
{

// Small helper to determine the container size
// we need to store n exponents in a dynamic packed
// monomial of type T. U must be an
// unsigned integral type.
template <typename T, typename U>
constexpr auto dpm_nexpos_to_vsize(const U &n) noexcept
{
    static_assert(is_integral_v<U> && !is_signed_v<U>);
    return n / T::psize + static_cast<U>(n % T::psize != 0u);
}

} // namespace detail

// Dynamic packed monomial.
#if defined(OBAKE_HAVE_CONCEPTS)
template <KPackable T, unsigned NBits>
    requires(NBits >= 3u)
    && (NBits <= static_cast<unsigned>(::obake::detail::limits_digits<T>))
#else
template <typename T, unsigned NBits,
          typename = ::std::enable_if_t<is_k_packable_v<T> && (NBits >= 3u)
                                        && (NBits <= static_cast<unsigned>(::obake::detail::limits_digits<T>))>>
#endif
        class d_packed_monomial
{
public:
    // The number of exponents to be packed into each T instance.
    static constexpr unsigned psize = static_cast<unsigned>(::obake::detail::limits_digits<T>) / NBits;

    // Alias for T.
    using value_type = T;

    // The container type.
    using container_t = ::boost::container::small_vector<T, 1>;

    // Default constructor.
    d_packed_monomial() = default;

    // Constructor from symbol set.
    explicit d_packed_monomial(const symbol_set &ss)
        : m_container(::obake::safe_cast<typename container_t::size_type>(
            detail::dpm_nexpos_to_vsize<d_packed_monomial>(ss.size())))
    {
    }

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
        explicit d_packed_monomial(It it, ::std::size_t n)
    {
        // Prepare the container.
        const auto vsize = detail::dpm_nexpos_to_vsize<d_packed_monomial>(n);
        m_container.resize(::obake::safe_cast<typename container_t::size_type>(vsize));

        ::std::size_t counter = 0;
        for (auto &out : m_container) {
            k_packer<T> kp(psize);

            // Keep packing until we get to psize or we have
            // exhausted the input values.
            auto j = 0u;
            for (; j < psize && counter < n; ++j, ++counter, ++it) {
                kp << ::obake::safe_cast<T>(*it);
            }

            // This will be executed only at the last iteration
            // of the for loop, and it will zero pad
            // the last element of the container if n does not
            // divide exactly psize.
            for (; j < psize; ++j) {
                kp << T(0);
            }

            out = kp.get();
        }
    }

private:
    struct input_it_ctor_tag {
    };
    template <typename It>
    explicit d_packed_monomial(input_it_ctor_tag, It b, It e)
    {
        while (b != e) {
            k_packer<T> kp(psize);

            auto j = 0u;
            for (; j < psize && b != e; ++j, ++b) {
                kp << ::obake::safe_cast<T>(*b);
            }

            for (; j < psize; ++j) {
                kp << T(0);
            }

            m_container.push_back(kp.get());
        }
    }

public:
    // Ctor from a pair of input iterators.
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
        explicit d_packed_monomial(It b, It e) : d_packed_monomial(input_it_ctor_tag{}, b, e)
    {
    }

    // Ctor from input range.
#if defined(OBAKE_HAVE_CONCEPTS)
    template <typename Range>
    requires InputRange<Range> &&SafelyCastable<typename ::std::iterator_traits<range_begin_t<Range>>::reference, T>
#else
    template <
        typename Range,
        ::std::enable_if_t<::std::conjunction_v<
                               is_input_range<Range>,
                               is_safely_castable<typename ::std::iterator_traits<range_begin_t<Range>>::reference, T>>,
                           int> = 0>
#endif
        explicit d_packed_monomial(Range &&r)
        : d_packed_monomial(input_it_ctor_tag{}, ::obake::begin(::std::forward<Range>(r)),
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
        explicit d_packed_monomial(::std::initializer_list<U> l)
        : d_packed_monomial(input_it_ctor_tag{}, l.begin(), l.end())
    {
    }

    container_t &_container()
    {
        return m_container;
    }
    const container_t &_container() const
    {
        return m_container;
    }

private:
    container_t m_container;
};

// Implementation of key_is_zero(). A monomial is never zero.
template <typename T, unsigned NBits>
inline bool key_is_zero(const d_packed_monomial<T, NBits> &, const symbol_set &)
{
    return false;
}

// Implementation of key_is_one(). A monomial is one if all its exponents are zero.
template <typename T, unsigned NBits>
inline bool key_is_one(const d_packed_monomial<T, NBits> &d, const symbol_set &)
{
    return ::std::all_of(d._container().cbegin(), d._container().cend(), [](const T &n) { return n == T(0); });
}

// Comparisons.
template <typename T, unsigned NBits>
inline bool operator==(const d_packed_monomial<T, NBits> &d1, const d_packed_monomial<T, NBits> &d2)
{
    return d1._container() == d2._container();
}

template <typename T, unsigned NBits>
inline bool operator!=(const d_packed_monomial<T, NBits> &d1, const d_packed_monomial<T, NBits> &d2)
{
    return !(d1 == d2);
}

// Hash implementation.
template <typename T, unsigned NBits>
inline ::std::size_t hash(const d_packed_monomial<T, NBits> &d)
{
    // NOTE: the idea is that we will mix the individual
    // hashes for every pack of exponents via addition.
    ::std::size_t ret = 0;
    for (const auto &n : d._container()) {
        ret += static_cast<::std::size_t>(n);
    }
    return ret;
}

// Symbol set compatibility implementation.
template <typename T, unsigned NBits>
inline bool key_is_compatible(const d_packed_monomial<T, NBits> &d, const symbol_set &s)
{
    const auto s_size = s.size();
    const auto &c = d._container();

    // Determine the size the container must have in order
    // to be able to represent s_size exponents.
    const auto exp_size = detail::dpm_nexpos_to_vsize<d_packed_monomial<T, NBits>>(s_size);

    // Check if c has the expected size.
    if (c.size() != exp_size) {
        return false;
    }

    // We need to check if the encoded values in the container
    // are within the limits. If NBits is maximal,
    // we don't need any checking as all encoded
    // values representable by T are allowed.
    if constexpr (NBits == ::obake::detail::limits_digits<T>) {
        return true;
    } else {
        // Fetch the elimits corresponding to a packing size of psize.
        const auto &e_lim = ::obake::detail::k_packing_get_elimits<T>(d_packed_monomial<T, NBits>::psize);
        for (const auto &n : c) {
            if constexpr (is_signed_v<T>) {
                if (n < e_lim[0] || n > e_lim[1]) {
                    return false;
                }
            } else {
                if (n > e_lim) {
                    return false;
                }
            }
        }

        return true;
    }
}

// Implementation of stream insertion.
// NOTE: requires that d is compatible with s.
template <typename T, unsigned NBits>
inline void key_stream_insert(::std::ostream &os, const d_packed_monomial<T, NBits> &d, const symbol_set &s)
{
    assert(polynomials::key_is_compatible(d, s));

    constexpr auto psize = d_packed_monomial<T, NBits>::psize;

    const auto &c = d._container();
    auto s_it = s.cbegin();
    const auto s_end = s.cend();

    T tmp;
    bool wrote_something = false;
    for (const auto &n : c) {
        k_unpacker<T> ku(n, psize);

        for (auto j = 0u; j < psize && s_it != s_end; ++j, ++s_it) {
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
                os << *s_it;
                wrote_something = true;
                if (tmp != T(1)) {
                    // The exponent is not unitary,
                    // print it.
                    os << "**" << ::obake::detail::to_string(tmp);
                }
            }
        }
    }

    if (!wrote_something) {
        // We did not write anything to the stream.
        // It means that all variables have zero
        // exponent, thus we print only "1".
        os << '1';
    }
}

// Implementation of tex stream insertion.
// NOTE: requires that d is compatible with s.
template <typename T, unsigned NBits>
inline void key_tex_stream_insert(::std::ostream &os, const d_packed_monomial<T, NBits> &d, const symbol_set &s)
{
    assert(polynomials::key_is_compatible(d, s));

    constexpr auto psize = d_packed_monomial<T, NBits>::psize;

    const auto &c = d._container();
    auto s_it = s.cbegin();
    const auto s_end = s.cend();

    // Use separate streams for numerator and denominator
    // (the denominator is used only in case of negative powers).
    ::std::ostringstream oss_num, oss_den, *cur_oss;

    T tmp;
    // Go through a multiprecision integer for the stream
    // insertion. This allows us not to care about potential
    // overflow conditions when manipulating the exponents
    // below.
    ::mppp::integer<1> tmp_mp;
    for (const auto &n : c) {
        k_unpacker<T> ku(n, psize);

        for (auto j = 0u; j < psize && s_it != s_end; ++j, ++s_it) {
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
                *cur_oss << '{' << *s_it << '}';

                // Raise to power, if the exponent is not one.
                if (!tmp_mp.is_one()) {
                    *cur_oss << "^{" << tmp_mp << '}';
                }
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
template <typename T, unsigned NBits>
inline d_packed_monomial<T, NBits> key_merge_symbols(const d_packed_monomial<T, NBits> &d,
                                                     const symbol_idx_map<symbol_set> &ins_map, const symbol_set &s)
{
    assert(polynomials::key_is_compatible(d, s));
    // The last element of the insertion map must be at most s.size(), which means that there
    // are symbols to be appended at the end.
    assert(ins_map.empty() || ins_map.rbegin()->first <= s.size());

    constexpr auto psize = d_packed_monomial<T, NBits>::psize;

    const auto &c = d._container();
    symbol_idx idx = 0;
    const auto s_size = s.size();
    auto map_it = ins_map.begin();
    const auto map_end = ins_map.end();
    T tmp;
    ::std::vector<T> tmp_v;
    for (const auto &n : c) {
        k_unpacker<T> ku(n, psize);

        for (auto j = 0u; j < psize && idx < s_size; ++j, ++idx) {
            if (map_it != map_end && map_it->first == idx) {
                // We reached an index at which we need to
                // insert new elements. Insert as many
                // zeroes as necessary in the temporary vector.
                tmp_v.insert(tmp_v.end(), ::obake::safe_cast<decltype(tmp_v.size())>(map_it->second.size()), T(0));
                // Move to the next element in the map.
                ++map_it;
            }

            // Add the existing element to tmp_v.
            ku >> tmp;
            tmp_v.push_back(tmp);
        }
    }

    assert(idx == s_size);

    // We could still have symbols which need to be appended at the end.
    if (map_it != map_end) {
        tmp_v.insert(tmp_v.end(), ::obake::safe_cast<decltype(tmp_v.size())>(map_it->second.size()), T(0));
        assert(map_it + 1 == map_end);
    }

    return d_packed_monomial<T, NBits>(tmp_v);
}

// Implementation of monomial_mul().
// NOTE: requires a, b and out to be compatible with ss.
template <typename T, unsigned NBits>
inline void monomial_mul(d_packed_monomial<T, NBits> &out, const d_packed_monomial<T, NBits> &a,
                         const d_packed_monomial<T, NBits> &b, [[maybe_unused]] const symbol_set &ss)
{
    // Verify the inputs.
    assert(polynomials::key_is_compatible(a, ss));
    assert(polynomials::key_is_compatible(b, ss));
    assert(polynomials::key_is_compatible(out, ss));

    // NOTE: check whether using pointers + restrict helps here
    // (in which case we'd have to add the requirement to monomial_mul()
    // that out must be distinct from a/b).
    ::std::transform(a._container().cbegin(), a._container().cend(), b._container().cbegin(), out._container().begin(),
                     [](const T &x, const T &y) { return x + y; });

    // Verify the output as well.
    assert(polynomials::key_is_compatible(out, ss));
}

} // namespace polynomials

// Lift to the obake namespace.
template <typename T, unsigned NBits>
using d_packed_monomial = polynomials::d_packed_monomial<T, NBits>;

// Specialise monomial_has_homomorphic_hash.
template <typename T, unsigned NBits>
inline constexpr bool monomial_hash_is_homomorphic<d_packed_monomial<T, NBits>> = true;

} // namespace obake

#endif
