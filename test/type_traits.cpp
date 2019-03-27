// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/type_traits.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cstddef>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>

#include <piranha/config.hpp>

using namespace piranha;

TEST_CASE("is_integral")
{
    REQUIRE(is_integral_v<int>);
    REQUIRE(is_integral_v<const int>);
    REQUIRE(is_integral_v<const volatile int>);
    REQUIRE(is_integral_v<volatile int>);
    REQUIRE(!is_integral_v<int &>);
    REQUIRE(!is_integral_v<const int &>);
    REQUIRE(!is_integral_v<int &&>);

    REQUIRE(is_integral_v<long>);
    REQUIRE(is_integral_v<const long>);
    REQUIRE(is_integral_v<const volatile long>);
    REQUIRE(is_integral_v<volatile long>);
    REQUIRE(!is_integral_v<long &>);
    REQUIRE(!is_integral_v<const long &>);
    REQUIRE(!is_integral_v<long &&>);

    REQUIRE(!is_integral_v<float>);
    REQUIRE(!is_integral_v<const double>);
    REQUIRE(!is_integral_v<const volatile long double>);
    REQUIRE(!is_integral_v<volatile float>);

    REQUIRE(!is_integral_v<std::string>);
    REQUIRE(!is_integral_v<void>);

#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(is_integral_v<__int128_t>);
    REQUIRE(is_integral_v<const __int128_t>);
    REQUIRE(is_integral_v<const volatile __int128_t>);
    REQUIRE(is_integral_v<volatile __int128_t>);
    REQUIRE(!is_integral_v<__int128_t &>);
    REQUIRE(!is_integral_v<const __int128_t &>);
    REQUIRE(!is_integral_v<__int128_t &&>);

    REQUIRE(is_integral_v<__uint128_t>);
    REQUIRE(is_integral_v<const __uint128_t>);
    REQUIRE(is_integral_v<const volatile __uint128_t>);
    REQUIRE(is_integral_v<volatile __uint128_t>);
    REQUIRE(!is_integral_v<__uint128_t &>);
    REQUIRE(!is_integral_v<const __uint128_t &>);
    REQUIRE(!is_integral_v<__uint128_t &&>);
#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(Integral<int>);
    REQUIRE(Integral<const int>);
    REQUIRE(Integral<const volatile int>);
    REQUIRE(Integral<volatile int>);
    REQUIRE(!Integral<int &>);
    REQUIRE(!Integral<const int &>);
    REQUIRE(!Integral<int &&>);
#endif
}

#if defined(PIRANHA_HAVE_CONCEPTS)
TEST_CASE("is_floating_point")
{
    REQUIRE(FloatingPoint<float>);
    REQUIRE(FloatingPoint<const float>);
    REQUIRE(FloatingPoint<const volatile float>);
    REQUIRE(FloatingPoint<volatile float>);
    REQUIRE(!FloatingPoint<float &>);
    REQUIRE(!FloatingPoint<const float &>);
    REQUIRE(!FloatingPoint<float &&>);
}
#endif

TEST_CASE("is_arithmetic")
{
    REQUIRE(is_arithmetic_v<int>);
    REQUIRE(is_arithmetic_v<const bool>);
    REQUIRE(is_arithmetic_v<const volatile float>);
    REQUIRE(is_arithmetic_v<volatile double>);
    REQUIRE(!is_arithmetic_v<float &>);
    REQUIRE(!is_arithmetic_v<const float &>);
    REQUIRE(!is_arithmetic_v<float &&>);

    REQUIRE(!is_arithmetic_v<std::string>);
    REQUIRE(!is_arithmetic_v<void>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(Arithmetic<float>);
    REQUIRE(Arithmetic<const bool>);
    REQUIRE(Arithmetic<const volatile long>);
    REQUIRE(Arithmetic<volatile char>);
    REQUIRE(!Arithmetic<float &>);
    REQUIRE(!Arithmetic<const int &>);
    REQUIRE(!Arithmetic<short &&>);
#endif
}

#if defined(PIRANHA_HAVE_CONCEPTS)
TEST_CASE("is_const")
{
    REQUIRE(!Const<void>);
    REQUIRE(Const<void const>);
    REQUIRE(Const<void const volatile>);
    REQUIRE(!Const<std::string>);
    REQUIRE(Const<std::string const>);
    REQUIRE(!Const<std::string &>);
    REQUIRE(!Const<const std::string &>);
}
#endif

TEST_CASE("is_signed")
{
    REQUIRE(!is_signed_v<void>);
    REQUIRE(!is_signed_v<unsigned>);
    REQUIRE(!is_signed_v<const unsigned>);
    REQUIRE(!is_signed_v<volatile unsigned>);
    REQUIRE(!is_signed_v<const volatile unsigned>);
    REQUIRE(is_signed_v<int>);
    REQUIRE(is_signed_v<const int>);
    REQUIRE(is_signed_v<volatile int>);
    REQUIRE(is_signed_v<const volatile int>);
    REQUIRE(!is_signed_v<int &>);
    REQUIRE(!is_signed_v<int &&>);
    REQUIRE(!is_signed_v<const int &>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(is_signed_v<__int128_t>);
    REQUIRE(is_signed_v<const __int128_t>);
    REQUIRE(is_signed_v<volatile __int128_t>);
    REQUIRE(is_signed_v<const volatile __int128_t>);
    REQUIRE(!is_signed_v<__int128_t &>);
    REQUIRE(!is_signed_v<__int128_t &&>);
    REQUIRE(!is_signed_v<const __int128_t &>);
    REQUIRE(!is_signed_v<__uint128_t>);
    REQUIRE(!is_signed_v<const __uint128_t>);
    REQUIRE(!is_signed_v<volatile __uint128_t>);
    REQUIRE(!is_signed_v<const volatile __uint128_t>);
    REQUIRE(!is_signed_v<__uint128_t &>);
    REQUIRE(!is_signed_v<__uint128_t &&>);
    REQUIRE(!is_signed_v<const __uint128_t &>);
#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!Signed<void>);
    REQUIRE(!Signed<void>);
    REQUIRE(!Signed<unsigned>);
    REQUIRE(!Signed<const unsigned>);
    REQUIRE(!Signed<volatile unsigned>);
    REQUIRE(!Signed<const volatile unsigned>);
    REQUIRE(Signed<int>);
    REQUIRE(Signed<const int>);
    REQUIRE(Signed<volatile int>);
    REQUIRE(Signed<const volatile int>);
    REQUIRE(!Signed<int &>);
    REQUIRE(!Signed<int &&>);
    REQUIRE(!Signed<const int &>);
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE(Signed<__int128_t>);
    REQUIRE(Signed<const __int128_t>);
    REQUIRE(Signed<volatile __int128_t>);
    REQUIRE(Signed<const volatile __int128_t>);
    REQUIRE(!Signed<__int128_t &>);
    REQUIRE(!Signed<__int128_t &&>);
    REQUIRE(!Signed<const __int128_t &>);
    REQUIRE(!Signed<__uint128_t>);
    REQUIRE(!Signed<const __uint128_t>);
    REQUIRE(!Signed<volatile __uint128_t>);
    REQUIRE(!Signed<const volatile __uint128_t>);
    REQUIRE(!Signed<__uint128_t &>);
    REQUIRE(!Signed<__uint128_t &&>);
    REQUIRE(!Signed<const __uint128_t &>);
#endif
#endif
}

TEST_CASE("make_unsigned")
{
    REQUIRE((std::is_same_v<unsigned, make_unsigned_t<unsigned>>));
    REQUIRE((std::is_same_v<unsigned, make_unsigned_t<int>>));
    REQUIRE((std::is_same_v<const unsigned, make_unsigned_t<const int>>));
    REQUIRE((std::is_same_v<volatile unsigned, make_unsigned_t<volatile int>>));
    REQUIRE((std::is_same_v<const volatile unsigned, make_unsigned_t<const volatile int>>));
    REQUIRE((std::is_same_v<unsigned, make_unsigned_t<unsigned>>));
    REQUIRE((std::is_same_v<const unsigned, make_unsigned_t<const unsigned>>));
    REQUIRE((std::is_same_v<volatile unsigned, make_unsigned_t<volatile unsigned>>));
    REQUIRE((std::is_same_v<const volatile unsigned, make_unsigned_t<const volatile unsigned>>));
#if defined(PIRANHA_HAVE_GCC_INT128)
    REQUIRE((std::is_same_v<__uint128_t, make_unsigned_t<__uint128_t>>));
    REQUIRE((std::is_same_v<const __uint128_t, make_unsigned_t<const __uint128_t>>));
    REQUIRE((std::is_same_v<volatile __uint128_t, make_unsigned_t<volatile __uint128_t>>));
    REQUIRE((std::is_same_v<const volatile __uint128_t, make_unsigned_t<const volatile __uint128_t>>));
    REQUIRE((std::is_same_v<__uint128_t, make_unsigned_t<__int128_t>>));
    REQUIRE((std::is_same_v<const __uint128_t, make_unsigned_t<const __int128_t>>));
    REQUIRE((std::is_same_v<volatile __uint128_t, make_unsigned_t<volatile __int128_t>>));
    REQUIRE((std::is_same_v<const volatile __uint128_t, make_unsigned_t<const volatile __int128_t>>));
#endif
}

struct unreturnable_00 {
    unreturnable_00(const unreturnable_00 &) = delete;
    unreturnable_00(unreturnable_00 &&) = delete;
};

struct unreturnable_01 {
    ~unreturnable_01() = delete;
};

TEST_CASE("is_returnable")
{
    REQUIRE(is_returnable_v<void>);
    REQUIRE(is_returnable_v<const void>);
    REQUIRE(is_returnable_v<volatile void>);
    REQUIRE(is_returnable_v<const volatile void>);
    REQUIRE(is_returnable_v<int>);
    REQUIRE(is_returnable_v<int &>);
    REQUIRE(is_returnable_v<const int &>);
    REQUIRE(is_returnable_v<int &&>);
    REQUIRE(is_returnable_v<int *>);
    REQUIRE(is_returnable_v<std::string>);
    REQUIRE(is_returnable_v<std::thread>);
    REQUIRE(is_returnable_v<std::unique_ptr<int>>);
    REQUIRE(is_returnable_v<std::shared_ptr<int>>);
    REQUIRE(!is_returnable_v<unreturnable_00>);
    REQUIRE(is_returnable_v<unreturnable_00 &>);
    REQUIRE(!is_returnable_v<unreturnable_01>);
    REQUIRE(is_returnable_v<unreturnable_01 &>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(Returnable<const volatile void>);
    REQUIRE(!Returnable<unreturnable_00>);
#endif
}

#if defined(PIRANHA_HAVE_CONCEPTS)
TEST_CASE("default_constructible")
{
    REQUIRE(DefaultConstructible<int>);
    REQUIRE(DefaultConstructible<int *>);
    REQUIRE(!DefaultConstructible<void>);
    REQUIRE(!DefaultConstructible<const void>);
    REQUIRE(!DefaultConstructible<int &>);
    REQUIRE(!DefaultConstructible<const int &>);
    REQUIRE(!DefaultConstructible<int &&>);
}
#endif

#if defined(PIRANHA_HAVE_CONCEPTS)
TEST_CASE("same")
{
    REQUIRE(Same<int, int>);
    REQUIRE(Same<void, void>);
    REQUIRE(!Same<void, const void>);
}
#endif

TEST_CASE("is_same_cvref")
{
    REQUIRE(is_same_cvref_v<int, int>);
    REQUIRE(is_same_cvref_v<int, int &>);
    REQUIRE(is_same_cvref_v<volatile int, int &>);
    REQUIRE(is_same_cvref_v<int &&, int &>);
    REQUIRE(is_same_cvref_v<const int &&, const int>);
    REQUIRE(is_same_cvref_v<int &, const int &>);
    REQUIRE(!is_same_cvref_v<void, int>);
    REQUIRE(!is_same_cvref_v<int, int *>);
    REQUIRE(!is_same_cvref_v<int *, int>);
    REQUIRE(is_same_cvref_v<void, void>);
    REQUIRE(is_same_cvref_v<void, const void>);
    REQUIRE(is_same_cvref_v<volatile void, const void>);
    REQUIRE(is_same_cvref_v<const volatile void, const void>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(SameCvref<int, int>);
    REQUIRE(SameCvref<int, int &>);
    REQUIRE(SameCvref<const int, int &>);
    REQUIRE(!SameCvref<int, void>);
    REQUIRE(!SameCvref<int *, int &>);
#endif
}

template <typename T, std::enable_if_t<is_string_like_v<T>, int> = 0>
void check_string_like_dispatch(const T &s)
{
    std::ostringstream o;
    o << s;
    REQUIRE(o.str() == s);
}

TEST_CASE("is_string_like_v")
{
    REQUIRE(!is_string_like_v<void>);
    REQUIRE(is_string_like_v<char *>);
    REQUIRE(is_string_like_v<const char *>);
    REQUIRE(is_string_like_v<char *const>);
    REQUIRE(is_string_like_v<const char *>);
    REQUIRE(is_string_like_v<const char *const>);
    REQUIRE(!is_string_like_v<char *&>);
    REQUIRE(is_string_like_v<char[]>);
    REQUIRE(is_string_like_v<const char[]>);
    REQUIRE(is_string_like_v<char[1]>);
    REQUIRE(is_string_like_v<const char[1]>);
    REQUIRE(is_string_like_v<char[2]>);
    REQUIRE(is_string_like_v<char[10]>);
    REQUIRE(is_string_like_v<const char[10]>);
    REQUIRE(!is_string_like_v<const char(&)[10]>);
    REQUIRE(!is_string_like_v<char(&)[10]>);
    REQUIRE(!is_string_like_v<char>);
    REQUIRE(!is_string_like_v<const char>);
    REQUIRE(!is_string_like_v<int>);
    REQUIRE(is_string_like_v<std::string>);
    REQUIRE(!is_string_like_v<std::string &>);
    REQUIRE(!is_string_like_v<const std::string &>);
    REQUIRE(is_string_like_v<const std::string>);
    REQUIRE(!is_string_like_v<char(&)[]>);
    REQUIRE(!is_string_like_v<char(&)[1]>);
    REQUIRE(is_string_like_v<const char[2]>);
    REQUIRE(!is_string_like_v<char(&&)[10]>);
    REQUIRE(is_string_like_v<std::string_view>);
    REQUIRE(!is_string_like_v<std::string_view &>);
    REQUIRE(!is_string_like_v<const std::string_view &>);
    REQUIRE(is_string_like_v<const std::string_view>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!StringLike<void>);
    REQUIRE(StringLike<char *>);
    REQUIRE(StringLike<const char *>);
    REQUIRE(!StringLike<char *&>);
    REQUIRE(!StringLike<const char(&)[10]>);
    REQUIRE(StringLike<std::string>);
    REQUIRE(StringLike<std::string_view>);
    REQUIRE(!StringLike<std::string &>);
#endif

    std::string s{"foo"};
    check_string_like_dispatch(std::string{"foo"});
    check_string_like_dispatch(s);
    check_string_like_dispatch(static_cast<const std::string &>(s));
    const char s1[] = "blab";
    check_string_like_dispatch(s1);
    check_string_like_dispatch(&s1[0]);
    check_string_like_dispatch("blab");
    char s2[] = "blab";
    check_string_like_dispatch(s2);
    check_string_like_dispatch(&s2[0]);
    const std::string_view sv1{"bubbbbba"};
    check_string_like_dispatch(sv1);
    std::string_view sv2{"bubbbba"};
    check_string_like_dispatch(sv2);
}

struct nonaddable_0 {
};

struct addable_0 {
    friend addable_0 operator+(addable_0, addable_0);
};

struct addable_1 {
    friend addable_1 operator+(addable_1, addable_0);
    friend addable_1 operator+(addable_0, addable_1);
};

struct nonaddable_1 {
    friend nonaddable_1 operator+(nonaddable_1, addable_0);
};

struct nonaddable_2 {
    friend nonaddable_2 operator+(nonaddable_2, addable_0);
    friend nonaddable_1 operator+(addable_0, nonaddable_2);
};

TEST_CASE("is_addable")
{
    REQUIRE(!is_addable_v<void>);
    REQUIRE(!is_addable_v<void, void>);
    REQUIRE(!is_addable_v<void, int>);
    REQUIRE(!is_addable_v<int, void>);
    REQUIRE(is_addable_v<int>);
    REQUIRE(is_addable_v<int, int>);
    REQUIRE(is_addable_v<const int, int &>);
    REQUIRE(is_addable_v<int &&, volatile int &>);
    REQUIRE(is_addable_v<std::string, char *>);
    REQUIRE(is_addable_v<std::string, char *>);
    REQUIRE(!is_addable_v<std::string, int>);
    REQUIRE(!is_addable_v<nonaddable_0>);
    REQUIRE(is_addable_v<addable_0>);
    REQUIRE(is_addable_v<addable_1, addable_0>);
    REQUIRE(!is_addable_v<nonaddable_1, addable_0>);
    REQUIRE(!is_addable_v<nonaddable_2, addable_0>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!Addable<void>);
    REQUIRE(!Addable<void, void>);
    REQUIRE(!Addable<void, int>);
    REQUIRE(!Addable<int, void>);
    REQUIRE(Addable<int>);
    REQUIRE(Addable<int, int>);
    REQUIRE(Addable<const int, int &>);
    REQUIRE(Addable<int &&, volatile int &>);
    REQUIRE(Addable<std::string, char *>);
    REQUIRE(Addable<std::string, char *>);
    REQUIRE(!Addable<std::string, int>);
    REQUIRE(!Addable<nonaddable_0>);
    REQUIRE(Addable<addable_0>);
    REQUIRE(Addable<addable_1, addable_0>);
    REQUIRE(!Addable<nonaddable_1, addable_0>);
    REQUIRE(!Addable<nonaddable_2, addable_0>);
#endif
}

struct noncomp_0 {
};

struct noncomp_1 {
    friend void operator==(const noncomp_1 &, const noncomp_1 &) {}
    friend void operator!=(const noncomp_1 &, const noncomp_1 &) {}
};

struct noncomp_2 {
    friend int operator==(const noncomp_2 &, const noncomp_2 &)
    {
        return 1;
    }
};

struct comp_0 {
    friend int operator==(const comp_0 &, const comp_0 &)
    {
        return 1;
    }
    friend int operator!=(const comp_0 &, const comp_0 &)
    {
        return 0;
    }
    friend int operator==(const comp_0 &, const noncomp_0 &)
    {
        return 1;
    }
    friend int operator!=(const comp_0 &, const noncomp_0 &)
    {
        return 0;
    }
};

TEST_CASE("is_equality_comparable")
{
    REQUIRE(!is_equality_comparable_v<void>);
    REQUIRE(!is_equality_comparable_v<void, void>);
    REQUIRE(!is_equality_comparable_v<int, void>);
    REQUIRE(!is_equality_comparable_v<void, int>);
    REQUIRE(is_equality_comparable_v<int>);
    REQUIRE(is_equality_comparable_v<int, int>);
    REQUIRE(is_equality_comparable_v<int, long>);
    REQUIRE(is_equality_comparable_v<std::string, std::string>);
    REQUIRE(!is_equality_comparable_v<std::string, int>);
    REQUIRE(!is_equality_comparable_v<int, std::string>);
    REQUIRE(!is_equality_comparable_v<noncomp_0>);
    REQUIRE(!is_equality_comparable_v<noncomp_1>);
    REQUIRE(!is_equality_comparable_v<noncomp_2>);
    REQUIRE(is_equality_comparable_v<comp_0>);
    REQUIRE(!is_equality_comparable_v<comp_0, noncomp_0>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!EqualityComparable<void>);
    REQUIRE(!EqualityComparable<void, void>);
    REQUIRE(!EqualityComparable<int, void>);
    REQUIRE(!EqualityComparable<void, int>);
    REQUIRE(EqualityComparable<int>);
    REQUIRE(EqualityComparable<int, int>);
    REQUIRE(EqualityComparable<int, long>);
    REQUIRE(EqualityComparable<std::string, std::string>);
    REQUIRE(!EqualityComparable<std::string, int>);
    REQUIRE(!EqualityComparable<int, std::string>);
    REQUIRE(!EqualityComparable<noncomp_0>);
    REQUIRE(!EqualityComparable<noncomp_1>);
    REQUIRE(!EqualityComparable<noncomp_2>);
    REQUIRE(EqualityComparable<comp_0>);
    REQUIRE(!EqualityComparable<comp_0, noncomp_0>);
#endif
}

TEST_CASE("is_pre_incrementable")
{
    REQUIRE(!is_pre_incrementable_v<void>);
    REQUIRE(is_pre_incrementable_v<int &>);
    REQUIRE(!is_pre_incrementable_v<const int &>);
    REQUIRE(!is_pre_incrementable_v<int &&>);
    REQUIRE(!is_pre_incrementable_v<std::string &>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!PreIncrementable<void>);
    REQUIRE(PreIncrementable<int &>);
    REQUIRE(!PreIncrementable<const int &>);
    REQUIRE(!PreIncrementable<int &&>);
    REQUIRE(!PreIncrementable<std::string &>);
#endif
}

// Boilerplate to test the arrow op type trait.
struct arrow01 {
    int *operator->();
};

struct arrow02 {
    arrow01 operator->();
    // NOTE: calling -> on a const instance will fail,
    // as it returns a non pointer type which does not
    // provide an operator->() member function.
    void operator->() const;
};

struct arrow03 {
    int operator->();
};

struct arrow03a {
    arrow02 operator->();
};

struct arrow04 {
    arrow03 operator->();
};

// Good forward iterator.
template <typename T>
struct fake_it_traits_forward {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::forward_iterator_tag;
};

// Broken reference type for forward it.
template <typename T>
struct fake_it_traits_forward_broken_ref {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = void;
    using iterator_category = std::forward_iterator_tag;
};

// Good output iterator.
template <typename T>
struct fake_it_traits_output {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::output_iterator_tag;
};

// Good input iterator.
template <typename T>
struct fake_it_traits_input {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::input_iterator_tag;
};

// Broken trait, incorrect category.
template <typename T>
struct fake_it_traits_wrong_tag {
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = void;
};

// Broken trait, missing typedefs.
template <typename T>
struct fake_it_traits_missing {
    using value_type = void;
    using pointer = void;
    using iterator_category = void;
};

#define PIRANHA_DECL_ITT_SPEC(it_type, trait_class)                                                                    \
    namespace std                                                                                                      \
    {                                                                                                                  \
    template <>                                                                                                        \
    struct iterator_traits<it_type> : trait_class {                                                                    \
    };                                                                                                                 \
    }

// Good input iterator.
struct iter01 {
    int &operator*() const;
    int *operator->() const;
    iter01 &operator++();
    iter01 &operator++(int);
    bool operator==(const iter01 &) const;
    bool operator!=(const iter01 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter01, fake_it_traits_input<int>)

// Good iterator, minimal requirements.
struct iter02 {
    int &operator*();
    iter02 &operator++();
};

PIRANHA_DECL_ITT_SPEC(iter02, fake_it_traits_input<int>)

// Broken iterator, minimal requirements.
struct iter03 {
    // int &operator*();
    iter03 &operator++();
};

PIRANHA_DECL_ITT_SPEC(iter03, fake_it_traits_input<int>)

// Broken iterator, minimal requirements.
struct iter04 {
    iter04 &operator=(const iter04 &) = delete;
    ~iter04() = delete;
    int &operator*();
    iter04 &operator++();
};

PIRANHA_DECL_ITT_SPEC(iter04, fake_it_traits_input<int>)

// Broken iterator, missing itt spec.
struct iter05 {
    int &operator*();
    iter05 &operator++();
};

// PIRANHA_DECL_ITT_SPEC(iter05,fake_it_traits_input<int>)

// Good input iterator: missing arrow, but the value type is not a class.
struct iter06 {
    int &operator*() const;
    // int *operator->();
    iter06 &operator++();
    iter06 &operator++(int);
    bool operator==(const iter06 &) const;
    bool operator!=(const iter06 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter06, fake_it_traits_input<int>)

struct iter06a_v {
};

// Bad input iterator: missing arrow, and the value type is a class.
struct iter06a {
    iter06a_v &operator*() const;
    // int *operator->();
    iter06a &operator++();
    iter06a &operator++(int);
    bool operator==(const iter06a &) const;
    bool operator!=(const iter06a &) const;
};

PIRANHA_DECL_ITT_SPEC(iter06a, fake_it_traits_input<iter06a_v>)

// Broken input iterator: missing equality.
struct iter07 {
    int &operator*() const;
    int *operator->() const;
    iter07 &operator++();
    iter07 &operator++(int);
    // bool operator==(const iter07 &) const;
    bool operator!=(const iter07 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter07, fake_it_traits_input<int>)

// Broken input iterator: missing itt spec.
struct iter08 {
    int &operator*() const;
    int *operator->() const;
    iter08 &operator++();
    iter08 &operator++(int);
    bool operator==(const iter08 &) const;
    bool operator!=(const iter08 &) const;
};

// PIRANHA_DECL_ITT_SPEC(iter08,fake_it_traits_input<int>)

// Good input iterator: broken arrow, but non-class.
struct iter09 {
    int &operator*() const;
    int operator->() const;
    iter09 &operator++();
    iter09 &operator++(int);
    bool operator==(const iter09 &) const;
    bool operator!=(const iter09 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter09, fake_it_traits_input<int>)

struct iter09a_v {
};

// Bad input iterator: broken arrow, and class value type.
struct iter09a {
    iter09a_v &operator*() const;
    iter09a_v operator->() const;
    iter09a &operator++();
    iter09a &operator++(int);
    bool operator==(const iter09a &) const;
    bool operator!=(const iter09a &) const;
};

PIRANHA_DECL_ITT_SPEC(iter09a, fake_it_traits_input<iter09a_v>)

// Good input iterator: multiple arrow.
struct iter10 {
    int &operator*() const;
    arrow03a operator->() const;
    iter10 &operator++();
    iter10 &operator++(int);
    bool operator==(const iter10 &) const;
    bool operator!=(const iter10 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter10, fake_it_traits_input<int>)

// Good input iterator: multiple broken arrow, but non-class.
struct iter11 {
    int &operator*() const;
    arrow04 operator->() const;
    iter11 &operator++();
    iter11 &operator++(int);
    bool operator==(const iter11 &) const;
    bool operator!=(const iter11 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter11, fake_it_traits_input<int>)

// Bad input iterator: inconsistent arrow / star, and class value.
struct foo_it_12 {
};

struct iter12_v {
};

struct iter12 {
    iter12_v &operator*() const;
    foo_it_12 *operator->() const;
    iter12 &operator++();
    iter12 &operator++(int);
    bool operator==(const iter12 &) const;
    bool operator!=(const iter12 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter12, fake_it_traits_input<iter12_v>)

// Good input iterator: different but compatible arrow / star.
struct iter13 {
    int operator*() const;
    int *operator->() const;
    iter13 &operator++();
    iter13 &operator++(int);
    bool operator==(const iter13 &) const;
    bool operator!=(const iter13 &) const;
};

// Specialise the it_traits for iter13 manually, as we need
// a custom reference type.
namespace std
{
template <>
struct iterator_traits<iter13> {
    using difference_type = std::ptrdiff_t;
    using value_type = int;
    using pointer = int *;
    using reference = int;
    using iterator_category = std::input_iterator_tag;
};
} // namespace std

// Good forward iterator.
struct iter14 {
    int &operator*() const;
    int *operator->() const;
    iter14 &operator++();
    iter14 &operator++(int);
    bool operator==(const iter14 &) const;
    bool operator!=(const iter14 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter14, fake_it_traits_forward<int>)

// Bad forward iterator: missing def ctor.
struct iter15 {
    iter15() = delete;
    int &operator*() const;
    int *operator->() const;
    iter15 &operator++();
    iter15 &operator++(int);
    bool operator==(const iter15 &) const;
    bool operator!=(const iter15 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter15, fake_it_traits_forward<int>)

// Bad forward iterator: not having reference types as reference in traits.
struct iter16 {
    int &operator*() const;
    int *operator->() const;
    iter16 &operator++();
    iter16 &operator++(int);
    bool operator==(const iter16 &) const;
    bool operator!=(const iter16 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter16, fake_it_traits_forward_broken_ref<int>)

// Bad forward iterator: broken tag in traits.
struct iter17 {
    int &operator*() const;
    int *operator->() const;
    iter17 &operator++();
    iter17 &operator++(int);
    bool operator==(const iter17 &) const;
    bool operator!=(const iter17 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter17, fake_it_traits_output<int>)

// Bad forward iterator: broken traits.
struct iter18 {
    int &operator*() const;
    int *operator->() const;
    iter18 &operator++();
    iter18 &operator++(int);
    bool operator==(const iter18 &) const;
    bool operator!=(const iter18 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter18, fake_it_traits_missing<int>)

// Bad forward iterator: broken ++.
struct iter19 {
    int &operator*() const;
    int *operator->() const;
    iter19 &operator++();
    void operator++(int);
    bool operator==(const iter19 &) const;
    bool operator!=(const iter19 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter19, fake_it_traits_forward<int>)

// Bad forward iterator: broken ++.
struct iter20 {
    int &operator*() const;
    int *operator->() const;
    void operator++();
    iter20 &operator++(int);
    bool operator==(const iter20 &) const;
    bool operator!=(const iter20 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter20, fake_it_traits_forward<int>)

struct iter21_v {
};

// Bad forward iterator: arrow returns type with different constness from star operator,
// and class value.
struct iter21 {
    iter21_v &operator*() const;
    const iter21_v *operator->() const;
    iter21 &operator++();
    iter21 &operator++(int);
    bool operator==(const iter21 &) const;
    bool operator!=(const iter21 &) const;
};

PIRANHA_DECL_ITT_SPEC(iter21, fake_it_traits_forward<iter21_v>)

#undef PIRANHA_DECL_ITT_SPEC

TEST_CASE("is_iterator")
{
    REQUIRE(!is_iterator_v<void>);
    REQUIRE(!is_iterator_v<int>);
    REQUIRE(is_iterator_v<int *>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!Iterator<void>);
#endif
}
