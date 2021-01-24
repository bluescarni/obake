// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_FW_UTILS_HPP
#define OBAKE_DETAIL_FW_UTILS_HPP

#include <cstddef>
#include <new>
#include <typeinfo>
#include <utility>

#include <boost/flyweight/holder_tag.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>

#include <obake/detail/visibility.hpp>

namespace obake::detail
{

OBAKE_DLL_PUBLIC ::std::pair<void *, bool> fw_fetch_storage(const ::std::type_info &, ::std::size_t, void (*)(void *));

[[noreturn]] OBAKE_DLL_PUBLIC void fw_handle_fatal_error();

// Implementation of a custom holder for flyweights. Largely lifted
// from the default static_holder:
//
// https://www.boost.org/doc/libs/1_74_0/boost/flyweight/static_holder.hpp
//
// The reason for this custom holder is that, in the
// presence of multiple DLLs using obake, we don't want to have
// multiple global factories for the flyweights, which
// would lead to crashes. That is, in this implementation the
// get() function will always return the same object, which is
// managed by a type-erased storage provider defined in fw_utils.cpp,
// even when get() is being invoked from multiple independent DLLs.
template <typename C>
struct fw_holder_class : ::boost::flyweights::holder_marker {
    // Ensure we don't try to use this with over-aligned classes.
    static_assert(alignof(C) <= alignof(::std::max_align_t));

    static C &impl()
    {
        // Try to fetch new or existing storage for an instance of type C.
        auto [storage, new_object]
            = detail::fw_fetch_storage(typeid(C), sizeof(C), [](void *ptr) { static_cast<C *>(ptr)->~C(); });

        if (new_object) {
            try {
                // New storage was allocated, need to create
                // a new object in it.
                auto ptr = ::new (storage) C;

                return *ptr;
                // LCOV_EXCL_START
            } catch (...) {
                // If the default constructor of C throws, there's
                // nothing we can do to recover.
                detail::fw_handle_fatal_error();
            }
            // LCOV_EXCL_STOP
        } else {
            // An instance of C was already created earlier
            // (i.e., the first time impl() was called).
            // Return a reference to it.
            return *static_cast<C *>(storage);
        }
    }

    static C &get()
    {
        static C &retval = fw_holder_class<C>::impl();

        return retval;
    }

    typedef fw_holder_class type;
    BOOST_MPL_AUX_LAMBDA_SUPPORT(1, fw_holder_class, (C))
};

struct fw_holder : ::boost::flyweights::holder_marker {
    template <typename C>
    struct apply {
        typedef fw_holder_class<C> type;
    };
};

} // namespace obake::detail

#endif
