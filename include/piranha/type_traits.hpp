// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_TYPE_TRAITS_HPP
#define PIRANHA_TYPE_TRAITS_HPP

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include <piranha/config.hpp>

namespace piranha
{

namespace detail
{

// http://en.cppreference.com/w/cpp/experimental/is_detected
template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
struct detector {
    using value_t = ::std::false_type;
    using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, ::std::void_t<Op<Args...>>, Op, Args...> {
    using value_t = ::std::true_type;
    using type = Op<Args...>;
};

// http://en.cppreference.com/w/cpp/experimental/nonesuch
struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(nonesuch const &) = delete;
    void operator=(nonesuch const &) = delete;
};

} // namespace detail

template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<detail::nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<detail::nonesuch, void, Op, Args...>::type;

template <template <class...> class Op, class... Args>
inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

// Handy alias.
template <typename T>
using remove_cvref_t = ::std::remove_cv_t<::std::remove_reference_t<T>>;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
PIRANHA_CONCEPT_DECL Same = ::std::is_same_v<T, U>;

#endif

// Detect if T and U, after the removal of reference and cv qualifiers, are the same type.
template <typename T, typename U>
using is_same_cvref = ::std::is_same<remove_cvref_t<T>, remove_cvref_t<U>>;

template <typename T, typename U>
inline constexpr bool is_same_cvref_v = is_same_cvref<T, U>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
PIRANHA_CONCEPT_DECL SameCvref = is_same_cvref_v<T, U>;

#endif

// Detect C++ integral types, including GCC-style 128bit integers.
template <typename T>
using is_integral = ::std::disjunction<::std::is_integral<T>
#if defined(PIRANHA_HAVE_GCC_INT128)
                                       ,
                                       ::std::is_same<::std::remove_cv_t<T>, __int128_t>,
                                       ::std::is_same<::std::remove_cv_t<T>, __uint128_t>
#endif
                                       >;

template <typename T>
inline constexpr bool is_integral_v = is_integral<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Integral = is_integral_v<T>;

#endif

#if defined(PIRANHA_HAVE_CONCEPTS)

// Concept for detecting C++ FP types.
template <typename T>
PIRANHA_CONCEPT_DECL FloatingPoint = ::std::is_floating_point_v<T>;

#endif

// Detect C++ arithmetic types, including GCC-style 128bit integers.
template <typename T>
using is_arithmetic = ::std::disjunction<is_integral<T>, ::std::is_floating_point<T>>;

template <typename T>
inline constexpr bool is_arithmetic_v = is_arithmetic<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Arithmetic = is_arithmetic_v<T>;

#endif

#if defined(PIRANHA_HAVE_CONCEPTS)

// Concept for detecting const-qualified types.
template <typename T>
PIRANHA_CONCEPT_DECL Const = ::std::is_const_v<T>;

#endif

// Detect (possibly cv-qualified) signed types.
// Supports also 128bit integers.
template <typename T>
using is_signed = ::std::disjunction<::std::is_signed<T>
#if defined(PIRANHA_HAVE_GCC_INT128)
                                     ,
                                     ::std::is_same<::std::remove_cv_t<T>, __int128_t>
#endif
                                     >;

template <typename T>
inline constexpr bool is_signed_v = is_signed<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Signed = is_signed_v<T>;

#endif

namespace detail
{

template <typename T, typename = void>
struct make_unsigned_impl : ::std::make_unsigned<T> {
    // NOTE: std::make_unsigned requires integrals but refuses bool:
    // https://en.cppreference.com/w/cpp/types/make_unsigned
    static_assert(!::std::is_same_v<bool, ::std::remove_cv_t<T>>,
                  "make_unsigned_t does not accept bool as input type.");
    static_assert(::std::is_integral_v<::std::remove_cv_t<T>> || ::std::is_enum_v<::std::remove_cv_t<T>>,
                  "make_unsigned_t works only on integrals or enumeration types.");
};

#if defined(PIRANHA_HAVE_GCC_INT128)

// NOTE: make_unsigned is supposed to preserve cv qualifiers, hence the non-trivial implementation.
template <typename T>
struct make_unsigned_impl<T,
                          ::std::enable_if_t<::std::disjunction_v<::std::is_same<::std::remove_cv_t<T>, __uint128_t>,
                                                                  ::std::is_same<::std::remove_cv_t<T>, __int128_t>>>> {
    using tmp_type = ::std::conditional_t<::std::is_const_v<T>, const __uint128_t, __uint128_t>;
    using type = ::std::conditional_t<::std::is_volatile_v<T>, volatile tmp_type, tmp_type>;
};

#endif

} // namespace detail

// Compute the corresponding unsigned type. Works on 128bit integers too.
template <typename T>
using make_unsigned_t = typename detail::make_unsigned_impl<T>::type;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL DefaultConstructible = ::std::is_default_constructible_v<T>;

#endif

// Detect semi-regular types.
template <typename T>
using is_semi_regular
    = ::std::conjunction<::std::is_default_constructible<T>, ::std::is_copy_constructible<T>,
                         ::std::is_move_constructible<T>, ::std::is_copy_assignable<T>, ::std::is_move_assignable<T>,
                         ::std::is_swappable<T>, ::std::is_destructible<T>>;

template <typename T>
inline constexpr bool is_semi_regular_v = is_semi_regular<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL SemiRegular = is_semi_regular_v<T>;

#endif

// Detect if type can be returned from a function.
// NOTE: constructability currently implies destructability:
// https://cplusplus.github.io/LWG/issue2116
// But it also seems like in the future the two concepts might
// be separated:
// https://en.cppreference.com/w/cpp/concepts/Constructible
// So require destructability explicitly as well.
template <typename T>
using is_returnable = ::std::disjunction<
    ::std::is_same<::std::remove_cv_t<T>, void>,
    ::std::conjunction<::std::is_destructible<T>,
                       ::std::disjunction<::std::is_copy_constructible<T>, ::std::is_move_constructible<T>>>>;

template <typename T>
inline constexpr bool is_returnable_v = is_returnable<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Returnable = is_returnable_v<T>;

#endif

namespace detail
{

// NOTE: std::remove_pointer_t removes the top level qualifiers of the pointer as well:
// http://en.cppreference.com/w/cpp/types/remove_pointer
// After removal of pointer, we could still have a type which is cv-qualified. Thus,
// we remove cv-qualifications after pointer removal.
template <typename T>
using is_char_pointer
    = ::std::conjunction<::std::is_pointer<T>, ::std::is_same<::std::remove_cv_t<::std::remove_pointer_t<T>>, char>>;

} // namespace detail

// Detect string-like types. As usual, cv qualifiers are ignored.
template <typename T>
using is_string_like = ::std::disjunction<
    // Is it std::string?
    ::std::is_same<::std::remove_cv_t<T>, ::std::string>,
    // Is it a char pointer?
    detail::is_char_pointer<T>,
    // Is it an array of chars?
    // NOTE: std::remove_cv_t does remove cv qualifiers from arrays.
    ::std::conjunction<::std::is_array<::std::remove_cv_t<T>>,
                       ::std::is_same<::std::remove_extent_t<::std::remove_cv_t<T>>, char>>,
    // Is it a string view?
    ::std::is_same<::std::remove_cv_t<T>, ::std::string_view>>;

template <typename T>
inline constexpr bool is_string_like_v = is_string_like<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL StringLike = is_string_like_v<T>;

#endif

namespace detail
{

template <typename T, typename U>
using add_t = decltype(::std::declval<T>() + ::std::declval<U>());

}

template <typename T, typename U = T>
using is_addable = ::std::conjunction<is_detected<detail::add_t, T, U>, is_detected<detail::add_t, U, T>,
                                      ::std::is_same<detected_t<detail::add_t, T, U>, detected_t<detail::add_t, U, T>>>;

template <typename T, typename U = T>
inline constexpr bool is_addable_v = is_addable<T, U>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U = T>
PIRANHA_CONCEPT_DECL Addable = requires(T &&x, U &&y)
{
    ::std::forward<T>(x) + ::std::forward<U>(y);
    ::std::forward<U>(y) + ::std::forward<T>(x);
    requires Same<decltype(::std::forward<T>(x) + ::std::forward<U>(y)),
                  decltype(::std::forward<U>(y) + ::std::forward<T>(x))>;
};

#endif

namespace detail
{

template <typename T>
using preinc_t = decltype(++::std::declval<T>());

}

// Pre-incrementable type-trait.
template <typename T>
using is_pre_incrementable = is_detected<detail::preinc_t, T>;

template <typename T>
inline constexpr bool is_pre_incrementable_v = is_pre_incrementable<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL PreIncrementable = requires(T &&x)
{
    ++::std::forward<T>(x);
};

#endif

namespace detail
{

template <typename T>
using postinc_t = decltype(::std::declval<T>()++);

}

// Post-incrementable type-trait.
template <typename T>
using is_post_incrementable = is_detected<detail::postinc_t, T>;

template <typename T>
inline constexpr bool is_post_incrementable_v = is_post_incrementable<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL PostIncrementable = requires(T &&x)
{
    ::std::forward<T>(x)++;
};

#endif

namespace detail
{

template <typename T, typename U>
using eq_t = decltype(::std::declval<T>() == ::std::declval<U>());

template <typename T, typename U>
using ineq_t = decltype(::std::declval<T>() != ::std::declval<U>());

} // namespace detail

// Equality-comparable type trait.
// NOTE: if the expressions above for eq/ineq return a type which is not bool,
// the decltype() will also check that the returned type is destructible.
template <typename T, typename U = T>
using is_equality_comparable = ::std::conjunction<::std::is_convertible<detected_t<detail::eq_t, T, U>, bool>,
                                                  ::std::is_convertible<detected_t<detail::eq_t, U, T>, bool>,
                                                  ::std::is_convertible<detected_t<detail::ineq_t, T, U>, bool>,
                                                  ::std::is_convertible<detected_t<detail::ineq_t, U, T>, bool>>;

template <typename T, typename U = T>
inline constexpr bool is_equality_comparable_v = is_equality_comparable<T, U>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U = T>
PIRANHA_CONCEPT_DECL EqualityComparable = requires(T &&x, U &&y)
{
    {
        ::std::forward<T>(x) == ::std::forward<U>(y)
    }
    ->bool;
    {
        ::std::forward<U>(y) == ::std::forward<T>(x)
    }
    ->bool;
    {
        ::std::forward<T>(x) != ::std::forward<U>(y)
    }
    ->bool;
    {
        ::std::forward<U>(y) != ::std::forward<T>(x)
    }
    ->bool;
};

#endif

namespace detail
{

// Helpers for the detection of the typedefs in std::iterator_traits.
// Use a macro (yuck) to reduce typing.
#define PIRANHA_DECLARE_IT_TRAITS_TYPE(type)                                                                           \
    template <typename T>                                                                                              \
    using it_traits_##type = typename ::std::iterator_traits<T>::type;

PIRANHA_DECLARE_IT_TRAITS_TYPE(difference_type)
PIRANHA_DECLARE_IT_TRAITS_TYPE(value_type)
PIRANHA_DECLARE_IT_TRAITS_TYPE(pointer)
PIRANHA_DECLARE_IT_TRAITS_TYPE(reference)
PIRANHA_DECLARE_IT_TRAITS_TYPE(iterator_category)

#undef PIRANHA_DECLARE_IT_TRAITS_TYPE

// Detect the availability of std::iterator_traits on type It.
template <typename It>
using has_iterator_traits
    = ::std::conjunction<is_detected<it_traits_reference, It>, is_detected<it_traits_value_type, It>,
                         is_detected<it_traits_pointer, It>, is_detected<it_traits_difference_type, It>,
                         is_detected<it_traits_iterator_category, It>>;

// All standard iterator tags packed in a tuple.
inline constexpr ::std::tuple<::std::input_iterator_tag, ::std::output_iterator_tag, ::std::forward_iterator_tag,
                              ::std::bidirectional_iterator_tag, ::std::random_access_iterator_tag>
    all_it_tags;

// Type resulting from the dereferencing operation.
template <typename T>
using deref_t = decltype(*::std::declval<T>());

// Check if the type T derives from one of the standard iterator tags.
// NOTE: MSVC has issues with the pattern below, adopt another implementation.
#if defined(_MSC_VER)

// NOTE: default empty for hard error (the default implementation is unused).
template <typename, typename>
struct derives_from_it_tag_impl {
};

template <typename T, typename... Args>
struct derives_from_it_tag_impl<T, ::std::tuple<Args...>> : ::std::disjunction<::std::is_base_of<Args, T>...> {
    static_assert(sizeof...(Args) > 0u, "Invalid parameter pack.");
};

template <typename T>
using derives_from_it_tag = derives_from_it_tag_impl<T, ::std::remove_const_t<decltype(all_it_tags)>>;

#else

template <typename T>
struct derives_from_it_tag {
    static constexpr bool value
        = ::std::apply([](auto... tag) { return (... || ::std::is_base_of_v<decltype(tag), T>); }, all_it_tags);
};

#endif

} // namespace detail

// Detect iterator types.
template <typename T>
using is_iterator = ::std::conjunction<
    // Copy constr/ass, destructible.
    ::std::is_copy_constructible<T>, ::std::is_copy_assignable<T>, ::std::is_destructible<T>,
    // Swappable.
    // NOTE: this adds lvalue refs to T, and becomes false if
    // T is void or a function type.
    ::std::is_swappable<T>,
    // Valid std::iterator_traits.
    detail::has_iterator_traits<T>,
    // Lvalue dereferenceable.
    is_detected<detail::deref_t, ::std::add_lvalue_reference_t<T>>,
    // Lvalue preincrementable, returning T &.
    ::std::is_same<detected_t<detail::preinc_t, ::std::add_lvalue_reference_t<T>>, ::std::add_lvalue_reference_t<T>>,
    // Add a check that the iterator category is one of the standard ones
    // or at least derives from it. This allows Boost.iterator iterators
    // (which have their own tags) to satisfy this type trait.
    detail::derives_from_it_tag<detected_t<detail::it_traits_iterator_category, T>>>;

template <typename T>
inline constexpr bool is_iterator_v = is_iterator<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL Iterator = is_iterator_v<T>;

#endif

namespace detail
{

// The purpose of these bits is to check whether U correctly implements the arrow operator.
// A correct implementation will return a pointer, after potentially calling
// the operator recursively as many times as needed. See:
// http://stackoverflow.com/questions/10677804/how-arrow-operator-overloading-works-internally-in-c

// The expression x->m is either:
// - equivalent to (*x).m, if x is a pointer, or
// - equivalent to (x.operator->())->m otherwise. That is, if operator->()
//   returns a pointer, then the member "m" of the pointee is returned,
//   otherwise there's a recursion to call again operator->() on the returned
//   value.
// This type trait will extract the final pointer type whose pointee type
// contains the "m" member.
template <typename, typename = void>
struct arrow_operator_type {
};

// Handy alias.
template <typename T>
using arrow_operator_t = typename arrow_operator_type<T>::type;

// If T is a pointer (after ref removal), we don't need to do anything: the final pointer type
// will be T itself (unreffed).
template <typename T>
struct arrow_operator_type<T, ::std::enable_if_t<::std::is_pointer_v<::std::remove_reference_t<T>>>> {
    using type = ::std::remove_reference_t<T>;
};

// Type resulting from the invocation of the member function operator->().
template <typename T>
using mem_arrow_op_t = decltype(::std::declval<T>().operator->());

// T is not a pointer, it is a class whose operator->() returns some type U.
// We call again arrow_operator_type on that U: if that leads eventually to a pointer
// (possibly by calling this specialisation recursively) then we define that pointer
// as the internal "type" member, otherwise we will SFINAE out.
template <typename T>
struct arrow_operator_type<T, ::std::enable_if_t<is_detected_v<arrow_operator_t, mem_arrow_op_t<T>>>> {
    using type = arrow_operator_t<mem_arrow_op_t<T>>;
};

// *it++ expression, used below.
template <typename T>
using it_inc_deref_t = decltype(*::std::declval<T>()++);

// The type resulting from dereferencing an lvalue of T,
// or nonesuch. Shortcut useful below.
template <typename T>
using det_deref_t = detected_t<deref_t, ::std::add_lvalue_reference_t<T>>;

// Deferred conditional. It will check the value of the
// compile-time boolean constant C, and derive from T if
// C is true, from F otherwise.
template <typename C, typename T, typename F>
struct dcond : ::std::conditional_t<C::value != false, T, F> {
};

} // namespace detail

// Input iterator type trait.
template <typename T>
using is_input_iterator = ::std::conjunction<
    // Must be a class or pointer.
    ::std::disjunction<::std::is_class<T>, ::std::is_pointer<T>>,
    // Base iterator requirements.
    is_iterator<T>,
    // Lvalue equality-comparable (just test the const-const variant).
    // NOTE: it's not clear here if we should test with rvalues as well. It seems like
    // the standard requires that "values" of T/const T need to be equality comparable:
    // https://en.cppreference.com/w/cpp/named_req/InputIterator
    // Which would seem to imply rvalues must be taken into account as well.
    // However, doing so for a binary operation adds a lot of combinatorial
    // possibilities. Moreover, according to these,
    // https://en.cppreference.com/w/cpp/named_req/InputIterator
    // https://en.cppreference.com/w/cpp/concepts/EqualityComparable
    // it seems like equality comparability for the C++20 input iterator
    // concept is being tested only with const lvalue refs on T. This technically
    // makes it possible to create a type which satisfies EqualityComparable
    // but fails if rvalues are used, e.g., via deletion of the equality
    // operator overload with rvalue refs:
    // https://godbolt.org/z/heaGiT
    // Thus, at least for the time being, we will be testing in this
    // and similar type traits only the "canonical" implementations
    // of a specific functionality and on lvalues, unless rvalues
    // are specifically required.
    is_equality_comparable<::std::add_lvalue_reference_t<const T>>,
    // Quoting the standard:
    // """
    // For every iterator type X for which equality is defined, there is a corresponding signed integer type called the
    // difference type of the iterator.
    // """
    // http://eel.is/c++draft/iterator.requirements#general-1
    // The equality comparable requirement appears in the input iterator requirements,
    // and it is then inherited by all the other iterator types (i.e., forward iterator,
    // bidir iterator, etc.).
    ::std::is_integral<detected_t<detail::it_traits_difference_type, T>>,
    ::std::is_signed<detected_t<detail::it_traits_difference_type, T>>,
    // *it returns it_traits::reference_type, both in mutable and const forms.
    // NOTE: it_traits::reference_type is never nonesuch, we tested its availability
    // in is_iterator.
    ::std::is_same<detail::det_deref_t<T>, detected_t<detail::it_traits_reference, T>>,
    ::std::is_same<detail::det_deref_t<const T>, detected_t<detail::it_traits_reference, T>>,
    // *it is convertible to it_traits::value_type.
    // NOTE: as above, it_traits::value_type does exist.
    ::std::is_convertible<detail::det_deref_t<T>, detected_t<detail::it_traits_value_type, T>>,
    ::std::is_convertible<detail::det_deref_t<const T>, detected_t<detail::it_traits_value_type, T>>,
    // it->m must be the same as (*it).m. What we test here is that the pointee type of the pointer type
    // yielded eventually by the arrow operator is the same as *it, but minus references: the arrow operator
    // always returns a pointer, but *it could return a new object (e.g., a transform iterator).
    // NOTE: we already verified earlier that T is dereferenceable, so deref_t will not be nonesuch.
    // NOTE: make this check conditional on whether the ref type is a class or not. If it's not a class,
    // no expression such as (*it).m is possible, and apparently some input iterators which are not
    // expected to point to classes do *not* implement the arrow operator as a consequence (e.g.,
    // see std::istreambuf_iterator).
    detail::dcond<
        ::std::is_class<::std::remove_reference_t<detail::det_deref_t<T>>>,
        ::std::conjunction<::std::is_same<::std::remove_reference_t<detail::det_deref_t<
                                              detected_t<detail::arrow_operator_t, ::std::add_lvalue_reference_t<T>>>>,
                                          ::std::remove_reference_t<detail::det_deref_t<T>>>,
                           ::std::is_same<::std::remove_reference_t<detail::det_deref_t<detected_t<
                                              detail::arrow_operator_t, ::std::add_lvalue_reference_t<const T>>>>,
                                          ::std::remove_reference_t<detail::det_deref_t<const T>>>>,
        ::std::true_type>,
    // ++it returns &it. Only non-const needed.
    ::std::is_same<detected_t<detail::preinc_t, ::std::add_lvalue_reference_t<T>>, ::std::add_lvalue_reference_t<T>>,
    // it is post-incrementable. Only non-const needed.
    is_post_incrementable<::std::add_lvalue_reference_t<T>>,
    // *it++ is convertible to the value type. Only non-const needed.
    ::std::is_convertible<detected_t<detail::it_inc_deref_t, ::std::add_lvalue_reference_t<T>>,
                          detected_t<detail::it_traits_value_type, T>>,
    // Check that the iterator category of T derives from the standard
    // input iterator tag. This accommodates the Boost iterators as well, who have
    // custom categories derived from the standard ones.
    ::std::is_base_of<::std::input_iterator_tag, detected_t<detail::it_traits_iterator_category, T>>>;

template <typename T>
inline constexpr bool is_input_iterator_v = is_input_iterator<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL InputIterator = is_input_iterator_v<T>;

#endif

namespace detail
{

template <typename T, typename U>
using out_iter_assign_t = decltype(*::std::declval<T>() = ::std::declval<U>());

template <typename T, typename U>
using out_iter_pia_t = decltype(*::std::declval<T>()++ = ::std::declval<U>());

} // namespace detail

// Output iterator type trait.
template <typename T, typename U>
using is_output_iterator = ::std::conjunction<
    // Must be a class or pointer.
    ::std::disjunction<::std::is_class<T>, ::std::is_pointer<T>>,
    // Must be an iterator.
    is_iterator<T>,
    // *r = o must be valid (r is an lvalue T, o is an U).
    is_detected<detail::out_iter_assign_t, ::std::add_lvalue_reference_t<T>, U>,
    // Lvalue pre-incrementable and returning lref to T.
    ::std::is_same<detected_t<detail::preinc_t, ::std::add_lvalue_reference_t<T>>, ::std::add_lvalue_reference_t<T>>,
    // Lvalue post-incrementable and returning convertible to const T &.
    ::std::is_convertible<detected_t<detail::postinc_t, ::std::add_lvalue_reference_t<T>>,
                          ::std::add_lvalue_reference_t<const T>>,
    // Can post-increment-assign on lvalue.
    is_detected<detail::out_iter_pia_t, ::std::add_lvalue_reference_t<T>, U>,
    // NOTE: if T is an input iterator, its category tag must *not* derive from std::output_iterator_tag
    // (the fact that it is an input iterator takes the precedence in the category tagging).
    // If T is a pure output iterator, its category tag must derive from std::output_iterator_tag.
    detail::dcond<is_input_iterator<T>,
                  ::std::negation<::std::is_base_of<::std::output_iterator_tag,
                                                    detected_t<detail::it_traits_iterator_category, T>>>,
                  ::std::is_base_of<::std::output_iterator_tag, detected_t<detail::it_traits_iterator_category, T>>>>;

template <typename T, typename U>
inline constexpr bool is_output_iterator_v = is_output_iterator<T, U>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
PIRANHA_CONCEPT_DECL OutputIterator = is_output_iterator_v<T, U>;

#endif

template <typename T>
using is_forward_iterator = ::std::conjunction<
    // Must be an input iterator.
    // NOTE: the pointer or class requirement is already in the input iterator.
    is_input_iterator<T>,
    // Must be def-ctible.
    ::std::is_default_constructible<T>,
    // If it is a mutable (i.e., output) iterator, it_traits::reference
    // must be a reference to the value type. Otherwise, it_traits::reference
    // must be a reference to const value type.
    // NOTE: we do not do the is_output_iterator check here, as we don't really know
    // what to put as a second template parameter.
    // NOTE: if the ref type is a mutable reference, then a forward iterator satisfies
    // also all the reqs of an output iterator.
    ::std::disjunction<
        ::std::is_same<detected_t<detail::it_traits_reference, T>,
                       ::std::add_lvalue_reference_t<detected_t<detail::it_traits_value_type, T>>>,
        ::std::is_same<detected_t<detail::it_traits_reference, T>,
                       ::std::add_lvalue_reference_t<const detected_t<detail::it_traits_value_type, T>>>>,
    // Post-incrementable lvalue returns convertible to const T &.
    ::std::is_convertible<detected_t<detail::postinc_t, ::std::add_lvalue_reference_t<T>>,
                          ::std::add_lvalue_reference_t<const T>>,
    // *r++ returns it_traits::reference.
    ::std::is_same<detected_t<detail::it_inc_deref_t, ::std::add_lvalue_reference_t<T>>,
                   detected_t<detail::it_traits_reference, T>>,
    // Category check.
    ::std::is_base_of<::std::forward_iterator_tag, detected_t<detail::it_traits_iterator_category, T>>>;

template <typename T>
inline constexpr bool is_forward_iterator_v = is_forward_iterator<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL ForwardIterator = is_forward_iterator_v<T>;

#endif

template <typename T>
using is_mutable_forward_iterator
    = ::std::conjunction<is_forward_iterator<T>,
                         ::std::is_same<detected_t<detail::it_traits_reference, T>,
                                        ::std::add_lvalue_reference_t<detected_t<detail::it_traits_value_type, T>>>>;

template <typename T>
inline constexpr bool is_mutable_forward_iterator_v = is_mutable_forward_iterator<T>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T>
PIRANHA_CONCEPT_DECL MutableForwardIterator = is_mutable_forward_iterator_v<T>;

#endif

namespace detail
{

template <typename T, typename... Args>
using function_object_call_t = decltype(::std::declval<T>()(::std::declval<Args>()...));

}

// Detect function objects:
// https://en.cppreference.com/w/cpp/named_req/FunctionObject
template <typename T, typename... Args>
using is_function_object
    = ::std::conjunction<::std::is_object<T>,
                         // NOTE: as explained in the input iterator type trait, we test lvalues here
                         // (both const and non-const).
                         is_detected<detail::function_object_call_t, ::std::add_lvalue_reference_t<T>, Args...>,
                         is_detected<detail::function_object_call_t, ::std::add_lvalue_reference_t<const T>, Args...>>;

template <typename T, typename... Args>
inline constexpr bool is_function_object_v = is_function_object<T, Args...>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename... Args>
PIRANHA_CONCEPT_DECL FunctionObject = is_function_object_v<T, Args...>;

#endif

// Detect if T is a hash function object
// for the type U:
// https://en.cppreference.com/w/cpp/named_req/Hash
// We also add the requirements from std::hash specialisations:
// https://en.cppreference.com/w/cpp/utility/hash
template <typename T, typename U>
using is_hash = ::std::conjunction<
    // T must be a semi-regular type.
    is_semi_regular<T>,
    // T must be a function object capable of taking an
    // lvalue of U as argument, and returning std::size_t.
    // NOTE: the Hash concept specifically talks about
    // lvalues in the cppreference page above.
    // NOTE: we have a slight duplication of is_function_object here,
    // but like this we avoid repeating the same checks multiple times.
    // NOTE: we ask for a call operator taking a const reference in input,
    // as that is the most natural implementation of a hash function (see
    // earlier discussions in is_function_object, is_iterator, etc.).
    ::std::is_object<T>,
    ::std::is_same<detected_t<detail::function_object_call_t, ::std::add_lvalue_reference_t<T>,
                              ::std::add_lvalue_reference_t<const U>>,
                   ::std::size_t>,
    ::std::is_same<detected_t<detail::function_object_call_t, ::std::add_lvalue_reference_t<const T>,
                              ::std::add_lvalue_reference_t<const U>>,
                   ::std::size_t>>;

template <typename T, typename U>
inline constexpr bool is_hash_v = is_hash<T, U>::value;

#if defined(PIRANHA_HAVE_CONCEPTS)

template <typename T, typename U>
PIRANHA_CONCEPT_DECL Hash = is_hash_v<T, U>;

#endif

} // namespace piranha

#endif
