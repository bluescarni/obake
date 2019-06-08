// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include <boost/container/container_fwd.hpp>

#include <piranha/detail/limits.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/symbols.hpp>

namespace piranha::detail
{

// Get a string representation of a symbol_set.
::std::string to_string(const symbol_set &s)
{
    ::std::string retval = "{";
    for (auto it = s.begin(); it != s.end(); ++it) {
        retval += '\'';
        retval += *it;
        retval += '\'';
        if (it + 1 != s.end()) {
            retval += ", ";
        }
    }
    retval += '}';

    return retval;
}

// Merge symbol sets.
::std::tuple<symbol_set, symbol_idx_map<symbol_set>, symbol_idx_map<symbol_set>> merge_symbol_sets(const symbol_set &s1,
                                                                                                   const symbol_set &s2)
{
    // Use the underlying sequence type
    // of symbol_set for the computation of the
    // union.
    symbol_set::sequence_type seq;

    // NOTE: the max size of the union is the sum of the two sizes, make sure
    // we can compute that safely.
    // NOTE: the size type of seq is the same size type of symbol_set.
    // LCOV_EXCL_START
    if (piranha_unlikely(s1.size() > ::std::get<1>(limits_minmax<symbol_set::size_type>) - s2.size())) {
        piranha_throw(::std::overflow_error,
                      "Overflow in the computation of the size of the union of two symbol sets of sizes "
                          + detail::to_string(s1.size()) + " and " + detail::to_string(s2.size()));
    }
    // LCOV_EXCL_STOP
    // Prepare the storage.
    seq.resize(static_cast<symbol_set::size_type>(s1.size() + s2.size()));

    // Do the union.
    const auto u_end = ::std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), seq.begin());
    // Get rid of the excess elements.
    seq.erase(u_end, seq.end());

    // Create the output set out of the union, knowing that it is ordered by construction.
    symbol_set u_set;
    u_set.adopt_sequence(::boost::container::ordered_unique_range_t{}, ::std::move(seq));

    // Small helper to compute the set difference between u_set
    // and the input symbol_set s.
    auto compute_map = [&u_set](const symbol_set &s) {
        symbol_idx_map<symbol_set> retval;

        // NOTE: max size of retval is the original size + 1
        // (there could be an insertion before each and every element of s, plus an
        // insertion at the end).
        // NOTE: here the static_cast is fine, even if the size type were
        // a short unsigned integral. The conversion rules ensure that the addition
        // is always done with unsigned arithmetic, thanks to the presence of 1u
        // as operand: if s.size() is short, it will be converted to int or unsigned,
        // if the conversion is to int it will be further converted to
        // unsigned, and the addition is always performed in at least unsigned
        // arithmetic.
        retval.reserve(static_cast<decltype(retval.size())>(s.size() + 1u));

        auto u_it = u_set.cbegin();
        for (decltype(s.size()) i = 0; i < s.size(); ++i, ++u_it) {
            assert(u_it != u_set.end());
            const auto &cur_sym = *s.nth(i);
            if (*u_it < cur_sym) {
                const auto new_it = retval.emplace_hint(retval.end(), i, symbol_set{*u_it});
                for (++u_it; *u_it < cur_sym; ++u_it) {
                    assert(u_it != u_set.end());
                    new_it->second.insert(new_it->second.end(), *u_it);
                }
                assert(*u_it == cur_sym);
            }
        }

        // Insertions at the end.
        if (u_it != u_set.cend()) {
            const auto new_it = retval.emplace_hint(retval.end(), s.size(), symbol_set{*u_it});
            for (++u_it; u_it != u_set.cend(); ++u_it) {
                new_it->second.insert(new_it->second.end(), *u_it);
            }
        }

        return retval;
    };

    auto m1 = compute_map(s1);
    auto m2 = compute_map(s2);

#if !defined(NDEBUG)
    auto check_map = [](const auto &m) {
        for (auto it = m.begin(); it != m.end(); ++it) {
            // There cannot be empty symbol sets in the map.
            assert(!it->second.empty());
            if (it + 1 != m.end()) {
                // If this is not the last pair in the map,
                // then its last symbol must precede, in
                // alphabetical order, the first symbol
                // of the next pair.
                assert(*(it->second.end() - 1) < *((it + 1)->second.begin()));
            }
        }
    };

    check_map(m1);
    check_map(m2);
#endif

    return ::std::make_tuple(::std::move(u_set), ::std::move(m1), ::std::move(m2));
}

} // namespace piranha::detail
