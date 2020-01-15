// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_ATOMIC_FLAG_ARRAY_HPP
#define OBAKE_DETAIL_ATOMIC_FLAG_ARRAY_HPP

#include <atomic>
#include <cstddef>
#include <memory>

#include <obake/detail/visibility.hpp>

namespace obake::detail
{

#if defined(_MSC_VER)

// NOTE: MSVC complains about the fact that we are using
// a class from the standard library (std::unique_ptr) in
// a DLL-exported class (atomic flag array). Because the
// unique_ptr is a private member of our class, we can
// safely ignore this warning:
// https://stackoverflow.com/questions/16419318/one-way-of-eliminating-c4251-warning-when-using-stl-classes-in-the-dll-interface/22054743
// https://stackoverflow.com/questions/2132747/warning-c4251-when-building-a-dll-that-exports-a-class-containing-an-atlcstrin
#pragma warning(push)
#pragma warning(disable : 4251)

#endif

// Helper to manage an array of atomic flags.
// The flags will all be cleared upon construction
// from a size.
class OBAKE_DLL_PUBLIC atomic_flag_array
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

#if defined(_MSC_VER)

#pragma warning(pop)

#endif

} // namespace obake::detail

#endif
