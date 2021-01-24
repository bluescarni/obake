// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include <obake/detail/fw_utils.hpp>

namespace obake::detail
{

namespace
{

// Storage provider for the flyweight machinery.
// This is a dictionary mapping a C++ type T to a dynamically-allocated
// array of uchars of size sizeof(T), and a function pointer.
// The array of uchars will be used to store a single instance of T,
// while the function is used to invoke the destructor of the T
// instance when the dict is being destroyed.
struct fw_storage_map {
    ::std::unordered_map<::std::type_index, ::std::tuple<::std::unique_ptr<unsigned char[]>, void (*)(void *)>> value;
    ~fw_storage_map()
    {
        for (auto &[_, tup] : value) {
            ::std::get<1>(tup)(static_cast<void *>(::std::get<0>(tup).get()));
        }
    }
};

// Helper to create on-demand the global
// objects used in the flyweight machinery.
// NOTE: perhaps it would be better to create
// these as global variables in the anonymous
// namespace, so that they get immediately
// created upon loading the library.
auto fw_statics()
{
    // Need a storage dict and a mutex to synchronize the
    // access to it.
    static fw_storage_map fw_map;
    static ::std::mutex fw_mutex;

    // NOTE: return a tuple of references.
    return ::std::make_tuple(::std::ref(fw_mutex), ::std::ref(fw_map));
}

} // namespace

// Fetch storage for an instance of type tp, whose size is s, and with a cleanup
// function f. The returned pointer will point to a chunk of either new or existing
// storage, the boolean flag indicates whether new storage was allocated or not.
// The function f will be used to destroy the instances stored in fw_map upon
// its destruction.
::std::pair<void *, bool> fw_fetch_storage(const ::std::type_info &tp, ::std::size_t s, void (*f)(void *))
{
    // Fetch references to the global objects.
    auto [fw_mutex, fw_map] = detail::fw_statics();

    // Lock for multithreading safety.
    ::std::lock_guard lock{fw_mutex};

    // Try to add a new entry for the type tp.
    const auto [it, new_object] = fw_map.value.try_emplace(tp);

    if (new_object) {
        // A new entry for tp was created. Allocate the storage
        // and register the cleanup function.
        try {
            ::std::get<0>(it->second) = ::std::unique_ptr<unsigned char[]>(new unsigned char[s]);
            // LCOV_EXCL_START
        } catch (...) {
            // If memory allocation fails, erase the just-added entry
            // before re-throwing.
            fw_map.value.erase(it);
            throw;
        }
        // LCOV_EXCL_STOP
        ::std::get<1>(it->second) = ::std::move(f);
    }

    return {::std::get<0>(it->second).get(), new_object};
}

// LCOV_EXCL_START

// Small helper to abort if we cannot default-construct
// C in fw_holder_class.
void fw_handle_fatal_error()
{
    ::std::cerr << "Fatal error in the implementation of a flyweight: the default-initialization of an object "
                   "in the holder class raised an exception"
                << ::std::endl;

    ::std::abort();
}

// LCOV_EXCL_STOP

} // namespace obake::detail
