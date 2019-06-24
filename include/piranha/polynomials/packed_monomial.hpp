// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)::polynomials
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_PACKED_MONOMIAL_HPP
#define PIRANHA_POLYNOMIALS_PACKED_MONOMIAL_HPP

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>
#include <piranha/detail/ignore.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/math/safe_cast.hpp>
#include <piranha/ranges.hpp>
#include <piranha/symbols.hpp>
#include <piranha/type_traits.hpp>
#include <piranha/utils/bit_packing.hpp>

namespace piranha
{

namespace polynomials
{

#if defined(PIRANHA_HAVE_CONCEPTS)
template <BitPackable T>
#else
template <typename T, typename = ::std::enable_if_t<is_bit_packable_v<T>>>
#endif
class packed_monomial
{
public:
    // Def ctor inits to a monomial with all zero exponents.
    constexpr packed_monomial() : m_value(0) {}
    // Constructor from symbol set.
    constexpr explicit packed_monomial(const symbol_set &) : packed_monomial() {}
    // Constructor from value.
    constexpr explicit packed_monomial(const T &n) : m_value(n) {}
    // Constructor from input iterator and size.
#if defined(PIRANHA_HAVE_CONCEPTS)
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
        bit_packer<T> bp(n);
        for (auto i = 0u; i < n; ++i, ++it) {
            bp << ::piranha::safe_cast<T>(*it);
        }
        m_value = bp.get();
    }

private:
    struct fwd_it_ctor_tag {
    };
    template <typename It>
    constexpr explicit packed_monomial(fwd_it_ctor_tag, It b, It e)
    {
        bit_packer<T> bp(::piranha::safe_cast<unsigned>(::std::distance(b, e)));
        for (; b != e; ++b) {
            bp << ::piranha::safe_cast<T>(*b);
        }
        m_value = bp.get();
    }

public:
    // Ctor from a pair of forward iterators.
#if defined(PIRANHA_HAVE_CONCEPTS)
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
#if defined(PIRANHA_HAVE_CONCEPTS)
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
        : packed_monomial(fwd_it_ctor_tag{}, ::piranha::begin(::std::forward<Range>(r)),
                          ::piranha::end(::std::forward<Range>(r)))
    {
    }
    // Ctor from init list.
#if defined(PIRANHA_HAVE_CONCEPTS)
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

    // The index for looking into the limits vectors
    // for the packed value. The elements at index
    // 0 in the limits vectors refer to size 1,
    // hence we must decrease by 1 the size.
    const auto idx = s_size - 1u;

    if constexpr (is_signed_v<T>) {
        const auto &mmp_arr = ::piranha::detail::sbp_get_mmp<T>();
        using size_type = decltype(mmp_arr.size());
        // Check that the size of the symbol set is not
        // too large for the current integral type,
        // and that the value of the monomial fits
        // within the boundaries of the packed
        // value for the given symbol set size.
        return idx < mmp_arr.size() && m.get_value() >= mmp_arr[static_cast<size_type>(idx)][0]
               && m.get_value() <= mmp_arr[static_cast<size_type>(idx)][1];
    } else {
        const auto &umax_arr = ::piranha::detail::ubp_get_max<T>();
        using size_type = decltype(umax_arr.size());
        return idx < umax_arr.size() && m.get_value() <= umax_arr[static_cast<size_type>(idx)];
    }
}

// Implementation of stream insertion.
// NOTE: requires that m is compatible with s.
template <typename T>
inline void key_stream_insert(::std::ostream &os, const packed_monomial<T> &m, const symbol_set &s)
{
    assert(::piranha::polynomials::key_is_compatible(m, s));

    // NOTE: we know s is not too large from the assert.
    const auto s_size = static_cast<unsigned>(s.size());
    bool wrote_something = false;
    bit_unpacker bu(m.get_value(), s_size);
    T tmp;

    for (const auto &var : s) {
        bu >> tmp;
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
                os << "**" << ::piranha::detail::to_string(tmp);
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

// Implementation of symbols merging.
// NOTE: requires that m is compatible with s, and ins_map consistent with s.
template <typename T>
inline packed_monomial<T> key_merge_symbols(const packed_monomial<T> &m, const symbol_idx_map<symbol_set> &ins_map,
                                            const symbol_set &s)
{
    assert(::piranha::polynomials::key_is_compatible(m, s));
    // The last element of the insertion map must be at most s.size(), which means that there
    // are symbols to be appended at the end.
    assert(ins_map.empty() || ins_map.rbegin()->first <= s.size());

    // Do a first pass to compute the total size after merging.
    auto merged_size = s.size();
    for (const auto &p : ins_map) {
        const auto tmp_size = p.second.size();
        // LCOV_EXCL_START
        if (piranha_unlikely(tmp_size
                             > ::std::get<1>(::piranha::detail::limits_minmax<decltype(s.size())>) - merged_size)) {
            piranha_throw(::std::overflow_error, "Overflow while trying to merge new symbols in a packed monomial: the "
                                                 "size of the merged monomial is too large");
        }
        // LCOV_EXCL_STOP
        merged_size += tmp_size;
    }

    // Init the unpacker and the packer.
    // NOTE: we know s.size() is small enough thanks to the
    // assertion at the beginning.
    bit_unpacker bu(m.get_value(), static_cast<unsigned>(s.size()));
    bit_packer<T> bp(::piranha::safe_cast<unsigned>(merged_size));

    auto map_it = ins_map.begin();
    const auto map_end = ins_map.end();
    for (auto i = 0u; i < static_cast<unsigned>(s.size()); ++i) {
        if (map_it != map_end && map_it->first == i) {
            // We reached an index at which we need to
            // insert new elements. Insert as many
            // zeroes as necessary in the packer.
            for (const auto &_ : map_it->second) {
                ::piranha::detail::ignore(_);
                bp << T(0);
            }
            // Move to the next element in the map.
            ++map_it;
        }
        // Add the existing element to the packer.
        T tmp;
        bu >> tmp;
        bp << tmp;
    }

    // We could still have symbols which need to be appended at the end.
    if (map_it != map_end) {
        for (const auto &_ : map_it->second) {
            ::piranha::detail::ignore(_);
            bp << T(0);
        }
        assert(map_it + 1 == map_end);
    }

    return packed_monomial<T>(bp.get());
}

} // namespace polynomials

// Lift to the piranha namespace.
template <typename T>
using packed_monomial = polynomials::packed_monomial<T>;

} // namespace piranha

#endif
