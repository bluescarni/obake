// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_UTILS_DEMANGLE_HPP
#define PIRANHA_UTILS_DEMANGLE_HPP

#include <string>
#include <type_traits>
#include <typeinfo>

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

#include <piranha/config.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace detail
{

inline ::std::string demangle_impl(const char *s)
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

} // namespace detail

// Demangler for type T. It will first strip reference and cv qualifications
// from T, and then re-decorate the resulting demangled name with the original
// qualifications. The reason we do this is because typeid ignores references
// and cv qualifications:
// http://en.cppreference.com/w/cpp/language/typeid
// See also this SO answer:
// https://stackoverflow.com/questions/28621844/is-there-a-typeid-for-references
template <typename T>
inline constexpr auto demangle = []() {
    // Get the uncvreffed demangled name.
    auto ret = []() -> ::std::string {
        using uncvref_T = remove_cvref_t<T>;
        // NOTE: on OSX, it seems like typeid() for 128bit types is not implemented.
        // Thus, we sidestep typeid() and provide directly the demangled
        // names of the bugged types. These are the same names returned on linux.
#if defined(PIRANHA_HAVE_GCC_INT128) && defined(__apple_build_version__)
        if constexpr (::std::is_same_v<uncvref_T, __int128_t>) {
            return "__int128";
        } else if constexpr (::std::is_same_v<uncvref_T, __int128_t *>) {
            return "__int128*";
        } else if constexpr (::std::is_same_v<uncvref_T, __int128_t const *>) {
            return "__int128 const*";
        } else if constexpr (::std::is_same_v<uncvref_T, __uint128_t>) {
            return "unsigned __int128";
        } else if constexpr (::std::is_same_v<uncvref_T, __uint128_t *>) {
            return "unsigned __int128*";
        } else if constexpr (::std::is_same_v<uncvref_T, __uint128_t const *>) {
            return "unsigned __int128 const*";
        } else {
#endif
            return detail::demangle_impl(typeid(uncvref_T).name());
#if defined(PIRANHA_HAVE_GCC_INT128) && defined(__apple_build_version__)
        }
#endif
    }();

    // Redecorate it with cv qualifiers.
    constexpr unsigned flag = unsigned(::std::is_const_v<::std::remove_reference_t<T>>)
                              + (unsigned(::std::is_volatile_v<::std::remove_reference_t<T>>) << 1);
    switch (flag) {
        case 0u:
            // NOTE: handle this explicitly to keep compiler warnings at bay.
            break;
        case 1u:
            ret += " const";
            break;
        case 2u:
            ret += " volatile";
            break;
        case 3u:
            ret += " const volatile";
    }
    // Re-add the reference, if necessary.
    if constexpr (::std::is_lvalue_reference_v<T>) {
        ret += " &";
    } else if constexpr (::std::is_rvalue_reference_v<T>) {
        ret += " &&";
    }
    return ret;
};

} // namespace piranha

#if defined(_MSC_VER)

#pragma warning(pop)

#endif

#endif
