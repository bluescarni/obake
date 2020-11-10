// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

#include <boost/container/container_fwd.hpp>
#include <boost/container_hash/hash.hpp>

#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/exceptions.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/symbols.hpp>

namespace obake::detail
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

// This function will merge the input sets s1 and s2, returning a tuple
// of three elements:
//
// - a set representing the union u of s1 and s2,
// - two insertion maps m1 and m2 representing the set differences u\\s1
//   and u\\s2 respectively.
//
// The insertion maps contain the indices in s1 and s2 at which symbols must be added
// so that s1 and s2, after the insertion of the symbols in m1 and m2, become identical to u.
//
// For example, given the input sets s1 = ["b", "c", "e"] and s2 = ["a", "c", "d", "f", "g"],
// the return values will be:
//
// - u = ["a", "b", "c", "d", "e", "f", "g"],
// - m1 = [(0, ["a"]), (2, ["d"]), (3, ["f", "g"])],
// - m2 = [(1, ["b"]), (3, ["e"])].
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
    if (obake_unlikely(s1.size() > limits_max<symbol_set::size_type> - s2.size())) {
        obake_throw(::std::overflow_error,
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

// This function first computes the intersection ix of the two sets s and s_ref, and then returns
// a set with the positional indices of ix in s_ref.
// NOTE: the implementation of this (and sm_intersect_idx()) can be probably improved
// performance wise (not sure about the use of lower_bound vs just a linear search,
// or perhaps using lower_bound vs plain loop when iterating over s might
// be worth it in certain situations, etc.). Need profiling data.
symbol_idx_set ss_intersect_idx(const symbol_set &s, const symbol_set &s_ref)
{
    // Use the underlying sequence type
    // of symbol_idx_set for the computation.
    symbol_idx_set::sequence_type seq;
    // Reserve storage. We won't ever need more than
    // the minimum between s.size() and s_ref.size().
    seq.reserve(::obake::safe_cast<decltype(seq.size())>(::std::min(s.size(), s_ref.size())));

    auto it = s_ref.begin();
    const auto e = s_ref.end();

    for (const auto &n : s) {
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
            seq.push_back(s_ref.index_of(it++));
        }
    }

#if !defined(NDEBUG)
    // Debug mode checks.
    // seq must be sorted.
    assert(::std::is_sorted(seq.cbegin(), seq.cend()));

    for (auto idx : seq) {
        // Every index in seq must be inside s_ref.
        assert(idx < s_ref.size());

        // The string at index idx in s_ref must be present in s.
        auto tmp_it = ::std::lower_bound(s.begin(), s.end(), *s_ref.nth(idx));
        assert(tmp_it != s.end());
        assert(*tmp_it == *s_ref.nth(idx));
    }

    for (const auto &n : s) {
        // Every element of s which is also in s_ref must have
        // an index in seq.
        const auto tmp_it = ::std::lower_bound(s_ref.begin(), s_ref.end(), n);
        if (tmp_it != s_ref.end() && *tmp_it == n) {
            const auto tmp_it_2 = ::std::lower_bound(seq.begin(), seq.end(), s_ref.index_of(tmp_it));
            assert(tmp_it_2 != seq.end());
        }
    }
#endif

    // Move seq into the return vale.
    symbol_idx_set retval;
    retval.adopt_sequence(::boost::container::ordered_unique_range_t{}, ::std::move(seq));

    return retval;
}

namespace
{

struct ss_fw_storage_map {
    ::std::unordered_map<::std::type_index,
                         ::std::tuple<::std::unique_ptr<unsigned char[]>, ::std::function<void(void *)>>>
        value;
    ~ss_fw_storage_map()
    {
        for (auto &[_, tup] : value) {
            std::cout << "Cleaning up storage\n";

            ::std::get<1>(tup)(static_cast<void *>(::std::get<0>(tup).get()));
        }
    }
};

auto ss_fw_statics()
{
    static ::std::mutex ss_fw_mutex;
    static ss_fw_storage_map ss_fw_map;

    return ::std::make_tuple(::std::ref(ss_fw_mutex), ::std::ref(ss_fw_map));
}

} // namespace

::std::pair<void *, bool> ss_fw_fetch_storage(const ::std::type_info &tp, ::std::size_t s,
                                              ::std::function<void(void *)> f)
{
    auto [ss_fw_mutex, ss_fw_map] = detail::ss_fw_statics();

    ::std::lock_guard lock{ss_fw_mutex};

    auto [it, new_object] = ss_fw_map.value.try_emplace(tp);

    std::cout << "Looking up storage for " << it->first.name() << '\n';

    if (new_object) {
        std::cout << "No storage found, creating new\n";
        // TODO exception safety in case of alloc failure.
        ::std::get<0>(it->second) = ::std::unique_ptr<unsigned char[]>(new unsigned char[s]);
        ::std::get<1>(it->second) = ::std::move(f);
    }

    return {::std::get<0>(it->second).get(), new_object};
}

::std::size_t ss_hasher::operator()(const symbol_set &ss) const
{
    ::std::size_t retval = 0;

    for (const auto &s : ss) {
        ::boost::hash_combine(retval, s);
    }

    return retval;
}

} // namespace obake::detail
