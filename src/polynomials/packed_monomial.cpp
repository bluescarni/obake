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
#include <utility>
#include <vector>

#include <fmt/core.h>

#include <mp++/integer.hpp>

#include <obake/config.hpp>
#include <obake/detail/ignore.hpp>
#include <obake/detail/limits.hpp>
#include <obake/exceptions.hpp>
#include <obake/kpack.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/polynomials/packed_monomial.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

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
                os << "**" << tmp;
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
    oss_num.flags(os.flags());
    oss_den.exceptions(::std::ios_base::failbit | ::std::ios_base::badbit);
    oss_den.flags(os.flags());

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
            *cur_oss << fmt::format("{{{}}}", var);

            // Raise to power, if the exponent is not one.
            if (!tmp_mp.is_one()) {
                *cur_oss << fmt::format("^{{{}}}", tmp_mp);
            }
        }
    }

    const auto num_str = oss_num.str(), den_str = oss_den.str();

    if (!num_str.empty() && !den_str.empty()) {
        // We have both negative and positive exponents,
        // print them both in a fraction.
        os << fmt::format("\\frac{{{}}}{{{}}}", num_str, den_str);
    } else if (!num_str.empty() && den_str.empty()) {
        // Only positive exponents.
        os << num_str;
    } else if (num_str.empty() && !den_str.empty()) {
        // Only negative exponents, display them as 1/something.
        os << fmt::format("\\frac{{1}}{{{}}}", den_str);
    } else {
        // We did not write anything to the stream.
        // It means that all variables have zero
        // exponent, thus we print only "1".
        assert(m.get_value() == T(0));
        os << '1';
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

namespace detail
{

namespace
{

// Implementation of key_degree().
// NOTE: this assumes that p is compatible with ss.
template <typename T>
T packed_monomial_key_degree(const packed_monomial<T> &p, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    T retval(0), tmp;
    kunpacker<T> ku(p.get_value(), s_size);
    for (auto i = 0u; i < s_size; ++i) {
        ku >> tmp;
        retval += tmp;
    }

    return retval;
}

} // namespace

} // namespace detail

::std::int32_t key_degree(const packed_monomial<::std::int32_t> &p, const symbol_set &ss)
{
    return detail::packed_monomial_key_degree(p, ss);
}

::std::uint32_t key_degree(const packed_monomial<::std::uint32_t> &p, const symbol_set &ss)
{
    return detail::packed_monomial_key_degree(p, ss);
}

#if defined(OBAKE_PACKABLE_INT64)

::std::int64_t key_degree(const packed_monomial<::std::int64_t> &p, const symbol_set &ss)
{
    return detail::packed_monomial_key_degree(p, ss);
}

::std::uint64_t key_degree(const packed_monomial<::std::uint64_t> &p, const symbol_set &ss)
{
    return detail::packed_monomial_key_degree(p, ss);
}

#endif

namespace detail
{

namespace
{

// Implementation of key_p_degree().
// NOTE: this assumes that p and si are compatible with ss.
template <typename T>
T packed_monomial_key_p_degree(const packed_monomial<T> &p, const symbol_idx_set &si, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    assert(si.empty() || *(si.end() - 1) < ss.size());

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    T retval(0), tmp;
    kunpacker<T> ku(p.get_value(), s_size);
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

} // namespace

} // namespace detail

::std::int32_t key_p_degree(const packed_monomial<::std::int32_t> &p, const symbol_idx_set &si, const symbol_set &ss)
{
    return detail::packed_monomial_key_p_degree(p, si, ss);
}

::std::uint32_t key_p_degree(const packed_monomial<::std::uint32_t> &p, const symbol_idx_set &si, const symbol_set &ss)
{
    return detail::packed_monomial_key_p_degree(p, si, ss);
}

#if defined(OBAKE_PACKABLE_INT64)

::std::int64_t key_p_degree(const packed_monomial<::std::int64_t> &p, const symbol_idx_set &si, const symbol_set &ss)
{
    return detail::packed_monomial_key_p_degree(p, si, ss);
}

::std::uint64_t key_p_degree(const packed_monomial<::std::uint64_t> &p, const symbol_idx_set &si, const symbol_set &ss)
{
    return detail::packed_monomial_key_p_degree(p, si, ss);
}

#endif

namespace detail
{

namespace
{

// Identify non-trimmable exponents in p.
// NOTE: this requires that p is compatible with ss,
// and that v has the same size as ss.
template <typename T>
void packed_monomial_key_trim_identify(::std::vector<int> &v, const packed_monomial<T> &p, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    assert(v.size() == ss.size());

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    kunpacker<T> ku(p.get_value(), s_size);
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

} // namespace

} // namespace detail

void key_trim_identify(::std::vector<int> &v, const packed_monomial<::std::int32_t> &p, const symbol_set &ss)
{
    return detail::packed_monomial_key_trim_identify(v, p, ss);
}

void key_trim_identify(::std::vector<int> &v, const packed_monomial<::std::uint32_t> &p, const symbol_set &ss)
{
    return detail::packed_monomial_key_trim_identify(v, p, ss);
}

#if defined(OBAKE_PACKABLE_INT64)

void key_trim_identify(::std::vector<int> &v, const packed_monomial<::std::int64_t> &p, const symbol_set &ss)
{
    return detail::packed_monomial_key_trim_identify(v, p, ss);
}

void key_trim_identify(::std::vector<int> &v, const packed_monomial<::std::uint64_t> &p, const symbol_set &ss)
{
    return detail::packed_monomial_key_trim_identify(v, p, ss);
}

#endif

namespace detail
{

namespace
{

// Eliminate from p the exponents at the indices
// specifed by si.
// NOTE: this requires that p is compatible with ss,
// and that si is consistent with ss.
template <typename T>
packed_monomial<T> packed_monomial_key_trim(const packed_monomial<T> &p, const symbol_idx_set &si, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    // NOTE: si cannot be larger than ss, and its last element must be smaller
    // than the size of ss.
    assert(si.size() <= ss.size() && (si.empty() || *(si.cend() - 1) < ss.size()));

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    kunpacker<T> ku(p.get_value(), s_size);
    kpacker<T> kp(static_cast<unsigned>(s_size - si.size()));
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

} // namespace

} // namespace detail

packed_monomial<::std::int32_t> key_trim(const packed_monomial<::std::int32_t> &p, const symbol_idx_set &si,
                                         const symbol_set &ss)
{
    return detail::packed_monomial_key_trim(p, si, ss);
}

packed_monomial<::std::uint32_t> key_trim(const packed_monomial<::std::uint32_t> &p, const symbol_idx_set &si,
                                          const symbol_set &ss)
{
    return detail::packed_monomial_key_trim(p, si, ss);
}

#if defined(OBAKE_PACKABLE_INT64)

packed_monomial<::std::int64_t> key_trim(const packed_monomial<::std::int64_t> &p, const symbol_idx_set &si,
                                         const symbol_set &ss)
{
    return detail::packed_monomial_key_trim(p, si, ss);
}

packed_monomial<::std::uint64_t> key_trim(const packed_monomial<::std::uint64_t> &p, const symbol_idx_set &si,
                                          const symbol_set &ss)
{
    return detail::packed_monomial_key_trim(p, si, ss);
}

#endif

namespace detail
{

namespace
{

// Monomial differentiation.
// NOTE: this requires that p is compatible with ss,
// and idx is within ss.
template <typename T>
::std::pair<T, packed_monomial<T>> packed_monomial_monomial_diff(const packed_monomial<T> &p, const symbol_idx &idx,
                                                                 const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    assert(idx < ss.size());

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    // Init the (un)packing machinery.
    kunpacker<T> ku(p.get_value(), s_size);
    kpacker<T> kp(s_size);
    T tmp, ret_exp(0);
    for (auto i = 0u; i < s_size; ++i) {
        ku >> tmp;

        if (i == idx && tmp != T(0)) {
            // NOTE: the exponent of the differentiation variable
            // is not zero. Take the derivative.
            // NOTE: if the exponent is zero, ret_exp will remain to
            // its initial value (0) and the output monomial
            // will be the same as p.
            // NOTE: no need for overflow checking here
            // due to the way we create the kpack deltas
            // and consequently the limits.
            ret_exp = tmp--;
        }

        kp << tmp;
    }

    return ::std::make_pair(ret_exp, packed_monomial<T>(kp.get()));
}

} // namespace

} // namespace detail

::std::pair<::std::int32_t, packed_monomial<::std::int32_t>> monomial_diff(const packed_monomial<::std::int32_t> &p,
                                                                           const symbol_idx &idx, const symbol_set &ss)
{
    return detail::packed_monomial_monomial_diff(p, idx, ss);
}

::std::pair<::std::uint32_t, packed_monomial<::std::uint32_t>>
monomial_diff(const packed_monomial<::std::uint32_t> &p, const symbol_idx &idx, const symbol_set &ss)
{
    return detail::packed_monomial_monomial_diff(p, idx, ss);
}

#if defined(OBAKE_PACKABLE_INT64)

::std::pair<::std::int64_t, packed_monomial<::std::int64_t>> monomial_diff(const packed_monomial<::std::int64_t> &p,
                                                                           const symbol_idx &idx, const symbol_set &ss)
{
    return detail::packed_monomial_monomial_diff(p, idx, ss);
}

::std::pair<::std::uint64_t, packed_monomial<::std::uint64_t>>
monomial_diff(const packed_monomial<::std::uint64_t> &p, const symbol_idx &idx, const symbol_set &ss)
{
    return detail::packed_monomial_monomial_diff(p, idx, ss);
}

#endif

namespace detail
{

namespace
{

// Monomial integration.
// NOTE: this requires that p is compatible with ss,
// and idx is within ss.
template <typename T>
::std::pair<T, packed_monomial<T>> packed_monomial_monomial_integrate(const packed_monomial<T> &p,
                                                                      const symbol_idx &idx, const symbol_set &ss)
{
    assert(polynomials::key_is_compatible(p, ss));
    assert(idx < ss.size());

    // NOTE: because we assume compatibility, the static cast is safe.
    const auto s_size = static_cast<unsigned>(ss.size());

    // Init the (un)packing machinery.
    kunpacker<T> ku(p.get_value(), s_size);
    kpacker<T> kp(s_size);
    T tmp, ret_exp(0);
    for (auto i = 0u; i < s_size; ++i) {
        ku >> tmp;

        if (i == idx) {
            if constexpr (is_signed_v<T>) {
                // For signed integrals, make sure
                // we are not integrating x**-1.
                if (obake_unlikely(tmp == T(-1))) {
                    obake_throw(::std::domain_error,
                                fmt::format("Cannot integrate a packed monomial: the exponent of the integration "
                                            "variable ('{}') is -1, "
                                            "and the integration would generate a logarithmic term",
                                            *ss.nth(static_cast<decltype(ss.size())>(i))));
                }
            }

            // NOTE: no need for overflow checking here
            // due to the way we create the kpack deltas
            // and consequently the limits.
            ret_exp = ++tmp;
        }

        kp << tmp;
    }
    // We must have written some nonzero value to ret_exp.
    assert(ret_exp != T(0));

    return ::std::make_pair(ret_exp, packed_monomial<T>(kp.get()));
}

} // namespace

} // namespace detail

::std::pair<::std::int32_t, packed_monomial<::std::int32_t>>
monomial_integrate(const packed_monomial<::std::int32_t> &p, const symbol_idx &idx, const symbol_set &ss)
{
    return detail::packed_monomial_monomial_integrate(p, idx, ss);
}

::std::pair<::std::uint32_t, packed_monomial<::std::uint32_t>>
monomial_integrate(const packed_monomial<::std::uint32_t> &p, const symbol_idx &idx, const symbol_set &ss)
{
    return detail::packed_monomial_monomial_integrate(p, idx, ss);
}

#if defined(OBAKE_PACKABLE_INT64)

::std::pair<::std::int64_t, packed_monomial<::std::int64_t>>
monomial_integrate(const packed_monomial<::std::int64_t> &p, const symbol_idx &idx, const symbol_set &ss)
{
    return detail::packed_monomial_monomial_integrate(p, idx, ss);
}

::std::pair<::std::uint64_t, packed_monomial<::std::uint64_t>>
monomial_integrate(const packed_monomial<::std::uint64_t> &p, const symbol_idx &idx, const symbol_set &ss)
{
    return detail::packed_monomial_monomial_integrate(p, idx, ss);
}

#endif

} // namespace polynomials

} // namespace obake
