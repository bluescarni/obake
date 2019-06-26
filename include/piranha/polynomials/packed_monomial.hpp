// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)::polynomials
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_PACKED_MONOMIAL_HPP
#define PIRANHA_POLYNOMIALS_PACKED_MONOMIAL_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <mp++/integer.hpp>

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
    // Alias for T.
    using value_type = T;

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
    assert(polynomials::key_is_compatible(m, s));

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
    assert(polynomials::key_is_compatible(m, s));
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

// Implementation of monomial_mul().
template <typename T>
constexpr void monomial_mul(packed_monomial<T> &out, const packed_monomial<T> &a, const packed_monomial<T> &b,
                            [[maybe_unused]] const symbol_set &ss)
{
    // Verify the inputs.
    assert(polynomials::key_is_compatible(a, ss));
    assert(polynomials::key_is_compatible(b, ss));

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
#if defined(PIRANHA_HAVE_CONCEPTS)
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
    auto b1 = ::piranha::begin(::std::forward<R1>(r1));
    const auto e1 = ::piranha::end(::std::forward<R1>(r1));
    auto b2 = ::piranha::begin(::std::forward<R2>(r2));
    const auto e2 = ::piranha::end(::std::forward<R2>(r2));

    if (b1 == e1 || b2 == e2) {
        // If either range is empty, there will be no
        // overflow, just return true;
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

    // Init with the first elements of the ranges.
    {
        // NOTE: if the iterators return copies
        // of the monomials, rather than references,
        // capturing them via const
        // ref will extend their lifetimes.
        const auto &init1 = *b1;
        const auto &init2 = *b2;

        assert(polynomials::key_is_compatible(init1, ss));
        assert(polynomials::key_is_compatible(init2, ss));

        bit_unpacker bu1(init1.get_value(), s_size);
        bit_unpacker bu2(init2.get_value(), s_size);
        value_type tmp;
        for (auto i = 0u; i < s_size; ++i) {
            bu1 >> tmp;
            if constexpr (is_signed_v<value_type>) {
                limits1.emplace_back(tmp, tmp);
            } else {
                limits1.emplace_back(tmp);
            }
            bu2 >> tmp;
            if constexpr (is_signed_v<value_type>) {
                limits2.emplace_back(tmp, tmp);
            } else {
                limits2.emplace_back(tmp);
            }
        }
    }

    // Examine the rest of the ranges.
    for (++b1; b1 != e1; ++b1) {
        const auto &cur = *b1;

        assert(polynomials::key_is_compatible(cur, ss));

        bit_unpacker bu(cur.get_value(), s_size);
        value_type tmp;
        for (decltype(limits1.size()) i = 0; i < s_size; ++i) {
            bu >> tmp;
            if constexpr (is_signed_v<value_type>) {
                limits1[i].first = ::std::min(limits1[i].first, tmp);
                limits1[i].second = ::std::max(limits1[i].second, tmp);
            } else {
                limits1[i] = ::std::max(limits1[i], tmp);
            }
        }
    }

    for (++b2; b2 != e2; ++b2) {
        const auto &cur = *b2;

        assert(polynomials::key_is_compatible(cur, ss));

        bit_unpacker bu(cur.get_value(), s_size);
        value_type tmp;
        for (decltype(limits2.size()) i = 0; i < s_size; ++i) {
            bu >> tmp;
            if constexpr (is_signed_v<value_type>) {
                limits2[i].first = ::std::min(limits2[i].first, tmp);
                limits2[i].second = ::std::max(limits2[i].second, tmp);
            } else {
                limits2[i] = ::std::max(limits2[i], tmp);
            }
        }
    }

    // Now add the limits via interval arithmetics
    // and check for overflow. Use mppp::integer for the check.
    if constexpr (is_signed_v<value_type>) {
        const auto &[_, min, max] = ::piranha::detail::sbp_get_minmax_elem<value_type>(s_size);
        ::piranha::detail::ignore(_);

        for (decltype(limits1.size()) i = 0; i < s_size; ++i) {
            const auto add_min = int_t{limits1[i].first} + limits2[i].first;
            const auto add_max = int_t{limits1[i].second} + limits2[i].second;

            if (add_min < min || add_max > max) {
                return false;
            }
        }
    } else {
        const auto &[_, max] = ::piranha::detail::ubp_get_max_elem<value_type>(s_size);
        ::piranha::detail::ignore(_);

        for (decltype(limits1.size()) i = 0; i < s_size; ++i) {
            const auto add_max = int_t{limits1[i]} + limits2[i];

            if (add_max > max) {
                return false;
            }
        }
    }

    return true;
}

} // namespace polynomials

// Lift to the piranha namespace.
template <typename T>
using packed_monomial = polynomials::packed_monomial<T>;

} // namespace piranha

#endif
