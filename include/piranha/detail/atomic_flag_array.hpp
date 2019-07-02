// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_ATOMIC_FLAG_ARRAY_HPP
#define PIRANHA_DETAIL_ATOMIC_FLAG_ARRAY_HPP

#include <atomic>
#include <cstddef>
#include <memory>

#include <piranha/detail/visibility.hpp>

namespace piranha::detail
{

// Helper to manage an array of atomic flags.
// The flags will all be cleared upon construction
// from a size.
class PIRANHA_DLL_PUBLIC atomic_flag_array
{
public:
    using value_type = ::std::atomic_flag;
    using size_type = ::std::size_t;

    explicit atomic_flag_array(size_type);
    // atomic_flag is guaranteed to have a trivial dtor,
    // so we can just let unique_ptr free the storage. No need
    // for custom dtor.
    // http://en.cppreference.com/w/cpp/atomic/atomic
    // Delete explicitly all other ctors/assignment operators.
    atomic_flag_array() = delete;
    atomic_flag_array(const atomic_flag_array &) = delete;
    atomic_flag_array(atomic_flag_array &&) = delete;
    atomic_flag_array &operator=(const atomic_flag_array &) = delete;
    atomic_flag_array &operator=(atomic_flag_array &&) = delete;

    // The accessors.
    value_type &operator[](const size_type &i)
    {
        return *reinterpret_cast<value_type *>(m_ptr.get() + sizeof(value_type) * i);
    }
    const value_type &operator[](const size_type &i) const
    {
        return *reinterpret_cast<const value_type *>(m_ptr.get() + sizeof(value_type) * i);
    }

private:
    ::std::unique_ptr<unsigned char[]> m_ptr;
    [[maybe_unused]] const size_type m_size;
};

} // namespace piranha::detail

#endif
