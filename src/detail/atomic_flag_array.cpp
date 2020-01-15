// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstddef>
#include <limits>
#include <new>

#include <obake/config.hpp>
#include <obake/detail/atomic_flag_array.hpp>
#include <obake/exceptions.hpp>

namespace obake::detail
{

atomic_flag_array::atomic_flag_array(size_type size) : m_size(size)
{
    // LCOV_EXCL_START
    if (obake_unlikely(size > ::std::numeric_limits<size_type>::max() / sizeof(value_type))) {
        obake_throw(::std::bad_alloc, );
    }
    // LCOV_EXCL_STOP

    // Dynamically create an array of unsigned char with enough storage.
    // This will throw bad_alloc in case the memory cannot be allocated.
    // NOTE: this is required to return memory sufficiently aligned for any type
    // which does not have extended alignment requirements:
    // https://stackoverflow.com/questions/10587879/does-new-char-actually-guarantee-aligned-memory-for-a-class-type
    m_ptr.reset(::new unsigned char[size * sizeof(value_type)]);
    // Now we use the unsigned char buffer to provide storage for the atomic flags. See:
    // http://eel.is/c++draft/intro.object
    // From now on, everything is noexcept.
    const auto end_ptr = m_ptr.get() + size * sizeof(value_type);
    for (auto ptr = m_ptr.get(); ptr != end_ptr; ptr += sizeof(value_type)) {
        // NOTE: atomic_flag should support aggregate init syntax:
        // http://en.cppreference.com/w/cpp/atomic/atomic
        // But it results in warnings, let's avoid initialisation
        // via ctor and just set the flag to false later.
        ::new (static_cast<void *>(ptr)) value_type;
        reinterpret_cast<value_type *>(ptr)->clear();
    }
}

} // namespace obake::detail
