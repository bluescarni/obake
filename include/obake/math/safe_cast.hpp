// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_MATH_SAFE_CAST_HPP
#define OBAKE_MATH_SAFE_CAST_HPP

#include <stdexcept>
#include <type_traits>
#include <utility>

#include <obake/config.hpp>
#include <obake/detail/visibility.hpp>
#include <obake/exceptions.hpp>
#include <obake/math/safe_convert.hpp>
#include <obake/type_name.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

// Exception to signal failure in obake::safe_cast().
class OBAKE_DLL_PUBLIC_INLINE_CLASS safe_cast_failure final : public ::std::invalid_argument
{
public:
    using ::std::invalid_argument::invalid_argument;
};

namespace detail
{

// NOTE: the use of From instead of From && in the concept is ok:
// - if x is an rvalue of type T, From resolves to T, then in the concept
//   we add back the &&, so we are checking the convertibility of an rvalue
//   reference to T;
// - is x is an lvalue of type T, From resolves to T &, in the concept we add
//   && which collapses to just T &, thus we are checking the convertibility of
//   an lvalue reference to T.
// This works as long as we are adding the && in the concept definition (or we
// are using std::declval<>() in the emulation layer).
template <typename To, typename From>
requires ::std::is_default_constructible_v<To> && SafelyConvertible<From, To &> && Returnable<To>
constexpr To safe_cast_impl(From &&x)
{
    // NOTE: value-initialisation allows us to use this function
    // in constexpr contexts. Note that in theory this may result in some overhead
    // with respect to omitting the curlies, however I hope that for
    // primitive types the compiler is smart enough to elide this initialisation
    // (as we will be writing into retval anyway when invoking safe_convert),
    // and for non-primitive types it's pretty likely value-initialisation
    // and default-initialisation are equivalent.
    To retval{};
    if (obake_unlikely(!::obake::safe_convert(retval, ::std::forward<From>(x)))) {
        obake_throw(safe_cast_failure, "A value of type '" + ::obake::type_name<From &&>()
                                           + "' could not be safely converted to the type '" + ::obake::type_name<To>()
                                           + "'");
    }
    return retval;
}

} // namespace detail

template <typename To>
inline constexpr auto safe_cast
    = [](auto &&x) OBAKE_SS_FORWARD_LAMBDA(detail::safe_cast_impl<To>(::std::forward<decltype(x)>(x)));

namespace detail
{

template <typename To, typename From>
using safe_cast_t = decltype(::obake::safe_cast<To>(::std::declval<From>()));

}

template <typename From, typename To>
using is_safely_castable = is_detected<detail::safe_cast_t, To, From>;

template <typename From, typename To>
inline constexpr bool is_safely_castable_v = is_safely_castable<From, To>::value;

template <typename From, typename To>
concept SafelyCastable = requires(From &&f)
{
    ::obake::safe_cast<To>(::std::forward<From>(f));
};

} // namespace obake

#endif
