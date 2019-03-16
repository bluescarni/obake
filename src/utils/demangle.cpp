// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>

#if defined(__GNUC__) || (defined(__clang__) && !defined(_MSC_VER))

// GCC demangle. This is available also for clang, both with libstdc++ and libc++.
#include <cstdlib>
#include <cxxabi.h>
#include <memory>

#elif defined(_MSC_VER)

// Disable some warnings for MSVC.
#pragma warning(push)
#pragma warning(disable : 4091)

#include <mutex>

// clang-format off
#include <Windows.h>
#include <Dbghelp.h>
// clang-format on

#endif

#include <piranha/utils/demangle.hpp>

namespace piranha::detail
{

::std::string demangle_impl(const char *s)
{
#if defined(__GNUC__) || (defined(__clang__) && !defined(_MSC_VER))
    int status = -4;
    // NOTE: abi::__cxa_demangle will return a pointer allocated by std::malloc, which we will delete via std::free.
    ::std::unique_ptr<char, void (*)(void *)> res{::abi::__cxa_demangle(s, nullptr, nullptr, &status), ::std::free};
    // NOTE: it seems like clang with libc++ does not set the status variable properly.
    // We then check if anything was allocated by __cxa_demangle(), as here it mentions
    // that in case of failure the pointer will be set to null:
    // https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html
    return res ? ::std::string(res.get()) : ::std::string(s);
#elif defined(_MSC_VER)
    // NOTE: the Windows function for demangling is not thread safe, we will have
    // to protect it with a mutex.
    // https://msdn.microsoft.com/ru-ru/library/windows/desktop/ms681400(v=vs.85).aspx
    // Local static init is thread safe in C++11.
    static ::std::mutex mut;
    char undecorated_name[1024];
    ::DWORD ret;
    {
        ::std::lock_guard lock{mut};
        ret = ::UnDecorateSymbolName(s, undecorated_name, sizeof(undecorated_name), UNDNAME_COMPLETE);
    }
    if (ret) {
        // Nonzero retval means success.
        return ::std::string(undecorated_name);
    }
    // Otherwise, return the mangled name.
    return ::std::string(s);
#else
    return ::std::string(s);
#endif
}

} // namespace piranha::detail

#if defined(_MSC_VER)

#pragma warning(pop)

#endif
