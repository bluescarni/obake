// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cassert>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <stdexcept>

#include <fmt/format.h>

#include <mp++/integer.hpp>

#include <obake/config.hpp>
#include <obake/detail/ignore.hpp>
#include <obake/detail/limits.hpp>
#include <obake/exceptions.hpp>
#include <obake/kpack.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/symbols.hpp>

namespace obake
{

namespace polynomials
{

namespace detail
{

namespace
{

// Implementation of stream insertion.
// NOTE: requires that m is compatible with s.
template <typename T>
void packed_monomial_stream_insert(::std::ostream &os, const packed_monomial<T> &m, const symbol_set &s)
{
    assert(polynomials::key_is_compatible(m, s));

    // NOTE: we know s is not too large from the assert.
    const auto s_size = static_cast<unsigned>(s.size());
    bool wrote_something = false;
    kunpacker<T> ku(m.get_value(), s_size);
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
                using namespace ::fmt::literals;
                os << "**{}"_format(tmp);
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

} // namespace

} // namespace detail

void key_stream_insert(::std::ostream &os, const packed_monomial<::std::int32_t> &m, const symbol_set &s)
{
    detail::packed_monomial_stream_insert(os, m, s);
}

void key_stream_insert(::std::ostream &os, const packed_monomial<::std::uint32_t> &m, const symbol_set &s)
{
    detail::packed_monomial_stream_insert(os, m, s);
}

#if defined(OBAKE_PACKABLE_INT64)

void key_stream_insert(::std::ostream &os, const packed_monomial<::std::int64_t> &m, const symbol_set &s)
{
    detail::packed_monomial_stream_insert(os, m, s);
}

void key_stream_insert(::std::ostream &os, const packed_monomial<::std::uint64_t> &m, const symbol_set &s)
{
    detail::packed_monomial_stream_insert(os, m, s);
}

#endif

namespace detail
{

namespace
{

// Implementation of tex stream insertion.
// NOTE: requires that m is compatible with s.
template <typename T>
void packed_monomial_tex_stream_insert(::std::ostream &os, const packed_monomial<T> &m, const symbol_set &s)
{
    assert(polynomials::key_is_compatible(m, s));

    // Use separate streams for numerator and denominator
    // (the denominator is used only in case of negative powers).
    ::std::ostringstream oss_num, oss_den, *cur_oss;
    oss_num.exceptions(::std::ios_base::failbit | ::std::ios_base::badbit);
    oss_den.exceptions(::std::ios_base::failbit | ::std::ios_base::badbit);

    // NOTE: we know s is not too large from the assert.
    const auto s_size = static_cast<unsigned>(s.size());
    kunpacker<T> ku(m.get_value(), s_size);
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

} // namespace

} // namespace detail

void key_tex_stream_insert(::std::ostream &os, const packed_monomial<::std::int32_t> &m, const symbol_set &s)
{
    detail::packed_monomial_tex_stream_insert(os, m, s);
}

void key_tex_stream_insert(::std::ostream &os, const packed_monomial<::std::uint32_t> &m, const symbol_set &s)
{
    detail::packed_monomial_tex_stream_insert(os, m, s);
}

#if defined(OBAKE_PACKABLE_INT64)

void key_tex_stream_insert(::std::ostream &os, const packed_monomial<::std::int64_t> &m, const symbol_set &s)
{
    detail::packed_monomial_tex_stream_insert(os, m, s);
}

void key_tex_stream_insert(::std::ostream &os, const packed_monomial<::std::uint64_t> &m, const symbol_set &s)
{
    detail::packed_monomial_tex_stream_insert(os, m, s);
}

#endif

namespace detail
{

namespace
{

// Implementation of symbols merging.
// NOTE: requires that m is compatible with s, and ins_map consistent with s.
template <typename T>
packed_monomial<T> packed_monomial_merge_symbols(const packed_monomial<T> &m, const symbol_idx_map<symbol_set> &ins_map,
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
    kunpacker<T> ku(m.get_value(), static_cast<unsigned>(s.size()));
    kpacker<T> kp(::obake::safe_cast<unsigned>(merged_size));

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

} // namespace

} // namespace detail

packed_monomial<::std::int32_t> key_merge_symbols(const packed_monomial<::std::int32_t> &m,
                                                  const symbol_idx_map<symbol_set> &ins_map, const symbol_set &s)
{
    return detail::packed_monomial_merge_symbols(m, ins_map, s);
}

packed_monomial<::std::uint32_t> key_merge_symbols(const packed_monomial<::std::uint32_t> &m,
                                                   const symbol_idx_map<symbol_set> &ins_map, const symbol_set &s)
{
    return detail::packed_monomial_merge_symbols(m, ins_map, s);
}

#if defined(OBAKE_PACKABLE_INT64)

packed_monomial<::std::int64_t> key_merge_symbols(const packed_monomial<::std::int64_t> &m,
                                                  const symbol_idx_map<symbol_set> &ins_map, const symbol_set &s)
{
    return detail::packed_monomial_merge_symbols(m, ins_map, s);
}

packed_monomial<::std::uint64_t> key_merge_symbols(const packed_monomial<::std::uint64_t> &m,
                                                   const symbol_idx_map<symbol_set> &ins_map, const symbol_set &s)
{
    return detail::packed_monomial_merge_symbols(m, ins_map, s);
}

#endif

} // namespace polynomials

} // namespace obake
