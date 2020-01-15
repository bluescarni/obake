// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_ATOMIC_LOCK_GUARD_HPP
#define OBAKE_DETAIL_ATOMIC_LOCK_GUARD_HPP

#include <atomic>

namespace obake::detail
{

// A simple spinlock built on top of std::atomic_flag. See for reference:
// http://en.cppreference.com/w/cpp/atomic/atomic_flag
// http://stackoverflow.com/questions/26583433/c11-implementation-of-spinlock-using-atomic
// The memory order specification is to squeeze out some extra performance with respect to the
// default behaviour of atomic types.
class atomic_lock_guard
{
public:
    explicit atomic_lock_guard(::std::atomic_flag &af) : m_af(af)
    {
        while (m_af.test_and_set(::std::memory_order_acquire)) {
        }
    }
    ~atomic_lock_guard()
    {
        m_af.clear(::std::memory_order_release);
    }
    // Delete explicitly all other ctors/assignment operators.
    atomic_lock_guard() = delete;
    atomic_lock_guard(const atomic_lock_guard &) = delete;
    atomic_lock_guard(atomic_lock_guard &&) = delete;
    atomic_lock_guard &operator=(const atomic_lock_guard &) = delete;
    atomic_lock_guard &operator=(atomic_lock_guard &&) = delete;

private:
    ::std::atomic_flag &m_af;
};

} // namespace obake::detail

#endif
