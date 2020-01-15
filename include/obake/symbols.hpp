// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_SYMBOLS_HPP
#define OBAKE_SYMBOLS_HPP

#include <algorithm>
#include <cassert>
#include <string>
#include <tuple>
#include <utility>

#include <boost/container/container_fwd.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/string.hpp>

#include <obake/detail/visibility.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/s11n.hpp>

namespace obake
{

// Set of symbols.
using symbol_set = ::boost::container::flat_set<::std::string>;

// Unsigned integral type for indexing into a symbol_set.
using symbol_idx = symbol_set::size_type;

// Set of symbol indices.
using symbol_idx_set = ::boost::container::flat_set<symbol_idx>;

// Map of symbols. This sorted data structure maps
// symbols to instances of T.
template <typename T>
using symbol_map = ::boost::container::flat_map<::std::string, T>;

// Map of symbol indices. This sorted data structure maps
// symbol_idx instances to instances of T.
template <typename T>
using symbol_idx_map = ::boost::container::flat_map<symbol_idx, T>;

namespace detail
{

OBAKE_DLL_PUBLIC ::std::string to_string(const symbol_set &);

OBAKE_DLL_PUBLIC ::std::tuple<symbol_set, symbol_idx_map<symbol_set>, symbol_idx_map<symbol_set>>
merge_symbol_sets(const symbol_set &, const symbol_set &);

OBAKE_DLL_PUBLIC symbol_idx_set ss_intersect_idx(const symbol_set &, const symbol_set &);

// This function first computes the intersection ix of the two sets of symbols in m and s_ref, and then returns
// a map in which the keys are the positional indices of ix in s_ref and the values are the values
// in m corresponding to the keys in ix.
template <typename T>
inline symbol_idx_map<T> sm_intersect_idx(const symbol_map<T> &m, const symbol_set &s_ref)
{
    // Use the underlying sequence type
    // of symbol_idx_map for the computation.
    typename symbol_idx_map<T>::sequence_type seq;
    // Reserve storage. We won't ever need more than
    // the minimum between m.size() and s_ref.size().
    seq.reserve(::obake::safe_cast<decltype(seq.size())>(::std::min(m.size(), s_ref.size())));

    auto it = s_ref.begin();
    const auto e = s_ref.end();

    for (const auto &p : m) {
        const auto &n = p.first;

        // Locate n in the current range of s_ref.
        // NOTE: after this, 'it' will point to either:
        // - end (in which case we just stop),
        // - an element equal to n (in which case
        //   we will later bump 'it' up),
        // - an element > n (which implies that
        //   the element before 'it' is < n).
        it = ::std::lower_bound(it, e, n);

        if (it == e) {
            // n is > any other string in s_ref,
            // no more searching is needed.
            break;
        }

        if (*it == n) {
            // n was located in s_ref. Compute its index,
            // and bump 'it' up so that we start the next search from the
            // next element in s_ref.
            seq.emplace_back(s_ref.index_of(it++), p.second);
        }
    }

#if !defined(NDEBUG)
    // Debug mode checks.
    // seq must be sorted.
    assert(
        ::std::is_sorted(seq.cbegin(), seq.cend(), [](const auto &p1, const auto &p2) { return p1.first < p2.first; }));

    for (const auto &p : seq) {
        auto idx = p.first;

        // Every index in seq must be inside s_ref.
        assert(idx < s_ref.size());

        // The string at index idx in s_ref must be present in m.
        auto tmp_it = ::std::lower_bound(m.begin(), m.end(), *s_ref.nth(idx),
                                         [](const auto &p, const auto &n) { return p.first < n; });
        assert(tmp_it != m.end());
        assert(tmp_it->first == *s_ref.nth(idx));
    }

    for (const auto &p : m) {
        const auto &n = p.first;

        // Every element of m which is also in s_ref must have
        // an index in seq.
        const auto tmp_it = ::std::lower_bound(s_ref.begin(), s_ref.end(), n);
        if (tmp_it != s_ref.end() && *tmp_it == n) {
            const auto tmp_it_2 = ::std::lower_bound(seq.begin(), seq.end(), s_ref.index_of(tmp_it),
                                                     [](const auto &p, const auto &idx) { return p.first < idx; });
            assert(tmp_it_2 != seq.end());
        }
    }
#endif

    // Move seq into the return vale.
    symbol_idx_map<T> retval;
    retval.adopt_sequence(::boost::container::ordered_unique_range_t{}, ::std::move(seq));

    return retval;
}

} // namespace detail

} // namespace obake

// Serialisation for symbol_set.
namespace boost::serialization
{

template <class Archive>
inline void save(Archive &ar, const ::obake::symbol_set &ss, unsigned)
{
    ar << ss.size();

    for (const auto &n : ss) {
        ar << n;
    }
}

template <class Archive>
inline void load(Archive &ar, ::obake::symbol_set &ss, unsigned)
{
    // Fetch the size.
    decltype(ss.size()) size;
    ar >> size;

    // Extract the underlying sequence from ss
    // and prepare its size.
    auto seq(ss.extract_sequence());
    seq.resize(size);

    // Fetch the symbol names from the archive.
    for (auto &n : seq) {
        ar >> n;
    }

    // Move the sequence back into ss.
    ss.adopt_sequence(::boost::container::ordered_unique_range_t{}, ::std::move(seq));
}

// Disable tracking for symbol_set.
template <>
struct tracking_level<::obake::symbol_set> : ::obake::detail::s11n_no_tracking<::obake::symbol_set> {
};

} // namespace boost::serialization

BOOST_SERIALIZATION_SPLIT_FREE(::obake::symbol_set)

#endif
