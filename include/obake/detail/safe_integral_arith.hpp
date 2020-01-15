// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_SAFE_INTEGRAL_ARITH_HPP
#define OBAKE_DETAIL_SAFE_INTEGRAL_ARITH_HPP

#include <stdexcept>
#include <string>
#include <type_traits>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/exceptions.hpp>
#include <obake/type_name.hpp>
#include <obake/type_traits.hpp>

// We have to do a bit of preprocessor woodwork to determine
// if we have the integer overflow builtins.

#if defined(OBAKE_COMPILER_IS_GCC)

// GCC >= 5 is good to go:
// https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html
// https://software.intel.com/en-us/forums/intel-c-compiler/topic/720757
// NOTE: the minimum GCC version supported in obake is 7.
#define OBAKE_HAVE_INTEGER_OVERFLOW_BUILTINS

#elif defined(__clang__)

// On Clang, we can test explicitly if the builtins are available with the
// __has_builtin() macro:
// http://releases.llvm.org/3.4/tools/clang/docs/LanguageExtensions.html
// NOTE: the typed overflow builtins are available since clang 3.4, the
// generic ones are available since clang 3.8.

#if __has_builtin(__builtin_add_overflow) && __has_builtin(__builtin_sub_overflow)

#define OBAKE_HAVE_INTEGER_OVERFLOW_BUILTINS

#endif

#endif

namespace obake::detail
{

// Little wrapper for creating the error message for the safe
// add/sub functions.
template <typename T>
inline ::std::string safe_int_arith_err(const char *op, T op1, T op2)
{
    return ::std::string("Overflow error in an integral ") + op + ": the operands' type is '" + ::obake::type_name<T>()
           + "', and the operands' values are " + detail::to_string(op1) + " and " + detail::to_string(op2);
}

#if defined(OBAKE_HAVE_INTEGER_OVERFLOW_BUILTINS)

template <typename T>
inline T safe_int_add(T a, T b)
{
    // A couple of compile-time checks.
    // First, these functions are supposed to be called only with integral types in input.
    static_assert(is_integral_v<T>, "This function needs integral types in input.");

    // Second, the overflow builtins do not work on bools (we have an explicit specialisation
    // for bools later).
    static_assert(!::std::is_same_v<T, bool>, "This function cannot be invoked with a bool argument.");

    T retval;
    if (obake_unlikely(__builtin_add_overflow(a, b, &retval))) {
        obake_throw(::std::overflow_error, detail::safe_int_arith_err("addition", a, b));
    }

    return retval;
}

template <typename T>
inline T safe_int_sub(T a, T b)
{
    static_assert(is_integral_v<T>, "This function needs integral types in input.");
    static_assert(!::std::is_same_v<T, bool>, "This function cannot be invoked with a bool argument.");

    T retval;
    if (obake_unlikely(__builtin_sub_overflow(a, b, &retval))) {
        obake_throw(::std::overflow_error, detail::safe_int_arith_err("subtraction", a, b));
    }

    return retval;
}

#else

// The add implementation based on std::numeric_limits.
template <typename T>
inline T safe_int_add_impl(T a, T b)
{
    if constexpr (is_signed_v<T>) {
        if (b >= T(0)) {
            if (obake_unlikely(a > limits_max<T> - b)) {
                obake_throw(::std::overflow_error, detail::safe_int_arith_err("addition", a, b));
            }
        } else {
            if (obake_unlikely(a < limits_min<T> - b)) {
                obake_throw(::std::overflow_error, detail::safe_int_arith_err("addition", a, b));
            }
        }
    } else {
        if (obake_unlikely(a > limits_max<T> - b)) {
            obake_throw(::std::overflow_error, detail::safe_int_arith_err("addition", a, b));
        }
    }

    return static_cast<T>(a + b);
}

// The sub implementation based on std::numeric_limits.
template <typename T>
inline T safe_int_sub_impl(T a, T b)
{
    if constexpr (is_signed_v<T>) {
        if (b <= T(0)) {
            if (obake_unlikely(a > limits_max<T> + b)) {
                obake_throw(::std::overflow_error, detail::safe_int_arith_err("subtraction", a, b));
            }
        } else {
            if (obake_unlikely(a < limits_min<T> + b)) {
                obake_throw(::std::overflow_error, detail::safe_int_arith_err("subtraction", a, b));
            }
        }
    } else {
        if (obake_unlikely(a < b)) {
            obake_throw(::std::overflow_error, detail::safe_int_arith_err("subtraction", a, b));
        }
    }

    return static_cast<T>(a - b);
}

template <typename T>
inline T safe_int_add(T a, T b)
{
    static_assert(is_integral_v<T>, "This function needs integral types in input.");
    static_assert(!::std::is_same_v<T, bool>, "This function cannot be invoked with a bool argument.");

    return detail::safe_int_add_impl(a, b);
}

template <typename T>
inline T safe_int_sub(T a, T b)
{
    static_assert(is_integral_v<T>, "This function needs integral types in input.");
    static_assert(!::std::is_same_v<T, bool>, "This function cannot be invoked with a bool argument.");

    return detail::safe_int_sub_impl(a, b);
}

#endif

// Let's special case bools, as the integer overflow builtins will not work with them.
template <>
inline bool safe_int_add(bool a, bool b)
{
    if (obake_unlikely(a && b)) {
        obake_throw(::std::overflow_error, detail::safe_int_arith_err("addition", a, b));
    }

    return (a + b) != 0;
}

template <>
inline bool safe_int_sub(bool a, bool b)
{
    if (obake_unlikely(!a && b)) {
        obake_throw(::std::overflow_error, detail::safe_int_arith_err("subtraction", a, b));
    }

    return (a - b) != 0;
}

} // namespace obake::detail

// Undefine the macro, if necessary, as we don't need it outside this file.
#if defined(OBAKE_HAVE_INTEGER_OVERFLOW_BUILTINS)

#undef OBAKE_HAVE_INTEGER_OVERFLOW_BUILTINS

#endif

#endif
