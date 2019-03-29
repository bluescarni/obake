// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/ranges.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <array>
#include <initializer_list>
#include <iterator>
#include <list>
#include <vector>

// Test constexpr capabilities.
constexpr std::array aint{1, 2, 3};

template <bool>
struct foo {
};

[[maybe_unused]] foo<piranha::begin(aint) != piranha::end(aint)> f;

namespace ns
{

// Good range via ADL.
struct range00 {
};

int *begin(const range00 &);
int *end(const range00 &);

// Good range via member functions.
struct range01 {
    int *begin() const;
    int *end() const;
};

// Good range, but not input range.
struct range02 {
    std::ostream_iterator<double> begin() const;
    std::ostream_iterator<double> end() const;
};

// Good range, but only mutable version.
struct range03 {
    std::ostream_iterator<double> begin();
    std::ostream_iterator<double> end();
};

// Input but not forward.
struct range04 {
    std::istreambuf_iterator<char> begin();
    std::istreambuf_iterator<char> end();
};

// Has begin/end, but invalid return type.
struct norange00 {
};

void begin(const norange00 &);
void end(const norange00 &);

// Missing end().
struct norange01 {
};

int *begin(const norange01 &);

// Missing begin().
struct norange02 {
};

int *end(const norange02 &);

// Mismatched iterators.
struct norange03 {
};

int *begin(const norange03 &);
double *end(const norange03 &);

} // namespace ns

TEST_CASE("ranges_test")
{
    REQUIRE(!piranha::is_range_v<void>);
    REQUIRE(!piranha::is_input_range_v<void>);
    REQUIRE(!piranha::is_forward_range_v<void>);
    REQUIRE(!piranha::is_mutable_forward_range_v<void>);
    REQUIRE(piranha::is_range_v<std::vector<int>>);
    REQUIRE(piranha::is_range_v<const std::vector<int>>);
    REQUIRE(piranha::is_range_v<std::vector<int> &>);
    REQUIRE(piranha::is_range_v<std::vector<int> &&>);
    REQUIRE(piranha::is_range_v<const std::vector<int> &>);
    REQUIRE(piranha::is_range_v<int(&)[3]>);
    REQUIRE(piranha::is_range_v<ns::range00>);
    REQUIRE(piranha::is_range_v<ns::range00 &>);
    REQUIRE(piranha::is_range_v<const ns::range00 &>);
    REQUIRE(piranha::is_mutable_forward_range_v<std::vector<int> &>);
    REQUIRE(!piranha::is_mutable_forward_range_v<const std::vector<int> &>);
    REQUIRE((!piranha::is_mutable_forward_range_v<std::initializer_list<int> &&>));
    REQUIRE(piranha::is_mutable_forward_range_v<std::list<int> &>);
    REQUIRE(!piranha::is_mutable_forward_range_v<const std::list<int> &>);
    REQUIRE(piranha::is_mutable_forward_range_v<int(&)[3]>);
    REQUIRE(!piranha::is_mutable_forward_range_v<const int(&)[3]>);
    REQUIRE(piranha::is_input_range_v<ns::range00>);
    REQUIRE(piranha::is_input_range_v<ns::range00 &>);
    REQUIRE(piranha::is_input_range_v<const ns::range00 &>);
    REQUIRE(piranha::is_forward_range_v<ns::range00>);
    REQUIRE(piranha::is_forward_range_v<ns::range00 &>);
    REQUIRE(piranha::is_forward_range_v<const ns::range00 &>);
    REQUIRE(piranha::is_range_v<ns::range01>);
    REQUIRE(piranha::is_range_v<ns::range01 &>);
    REQUIRE(piranha::is_range_v<const ns::range01 &>);
    REQUIRE(piranha::is_range_v<ns::range02>);
    REQUIRE(piranha::is_range_v<ns::range02 &>);
    REQUIRE(piranha::is_range_v<const ns::range02 &>);
    REQUIRE(!piranha::is_input_range_v<ns::range02>);
    REQUIRE(!piranha::is_input_range_v<ns::range02 &>);
    REQUIRE(!piranha::is_input_range_v<const ns::range02 &>);
    REQUIRE(!piranha::is_range_v<ns::range03>);
    REQUIRE(!piranha::is_input_range_v<ns::range03>);
    REQUIRE(!piranha::is_forward_range_v<ns::range03>);
    REQUIRE(!piranha::is_mutable_forward_range_v<ns::range03>);
    REQUIRE(piranha::is_range_v<ns::range03 &>);
    REQUIRE(!piranha::is_range_v<const ns::range03 &>);
    REQUIRE(piranha::is_input_range_v<ns::range04 &>);
    REQUIRE(!piranha::is_forward_range_v<ns::range04 &>);
    REQUIRE(!piranha::is_range_v<ns::norange00>);
    REQUIRE(!piranha::is_input_range_v<ns::norange00>);
    REQUIRE(!piranha::is_forward_range_v<ns::norange00>);
    REQUIRE(!piranha::is_mutable_forward_range_v<ns::norange00>);
    REQUIRE(!piranha::is_range_v<ns::norange00 &>);
    REQUIRE(!piranha::is_range_v<const ns::norange00 &>);
    REQUIRE(!piranha::is_range_v<ns::norange01>);
    REQUIRE(!piranha::is_range_v<ns::norange01 &>);
    REQUIRE(!piranha::is_range_v<const ns::norange01 &>);
    REQUIRE(!piranha::is_range_v<ns::norange02>);
    REQUIRE(!piranha::is_range_v<ns::norange02 &>);
    REQUIRE(!piranha::is_range_v<const ns::norange02 &>);
    REQUIRE(!piranha::is_range_v<ns::norange03>);
    REQUIRE(!piranha::is_range_v<ns::norange03 &>);
    REQUIRE(!piranha::is_range_v<const ns::norange03 &>);

#if defined(PIRANHA_HAVE_CONCEPTS)
    REQUIRE(!piranha::Range<void>);
    REQUIRE(!piranha::InputRange<void>);
    REQUIRE(!piranha::ForwardRange<void>);
    REQUIRE(!piranha::MutableForwardRange<void>);
    REQUIRE(piranha::Range<std::vector<int>>);
    REQUIRE(piranha::Range<const std::vector<int>>);
    REQUIRE(piranha::Range<std::vector<int> &>);
    REQUIRE(piranha::Range<std::vector<int> &&>);
    REQUIRE(piranha::Range<const std::vector<int> &>);
    REQUIRE(piranha::Range<int(&)[3]>);
    REQUIRE(piranha::Range<ns::range00>);
    REQUIRE(piranha::Range<ns::range00 &>);
    REQUIRE(piranha::Range<const ns::range00 &>);
    REQUIRE(piranha::MutableForwardRange<std::vector<int> &>);
    REQUIRE(!piranha::MutableForwardRange<const std::vector<int> &>);
    REQUIRE((!piranha::MutableForwardRange<std::initializer_list<int> &&>));
    REQUIRE(piranha::MutableForwardRange<std::list<int> &>);
    REQUIRE(!piranha::MutableForwardRange<const std::list<int> &>);
    REQUIRE(piranha::MutableForwardRange<int(&)[3]>);
    REQUIRE(!piranha::MutableForwardRange<const int(&)[3]>);
    REQUIRE(piranha::InputRange<ns::range00>);
    REQUIRE(piranha::InputRange<ns::range00 &>);
    REQUIRE(piranha::InputRange<const ns::range00 &>);
    REQUIRE(piranha::ForwardRange<ns::range00>);
    REQUIRE(piranha::ForwardRange<ns::range00 &>);
    REQUIRE(piranha::ForwardRange<const ns::range00 &>);
    REQUIRE(piranha::Range<ns::range01>);
    REQUIRE(piranha::Range<ns::range01 &>);
    REQUIRE(piranha::Range<const ns::range01 &>);
    REQUIRE(piranha::Range<ns::range02>);
    REQUIRE(piranha::Range<ns::range02 &>);
    REQUIRE(piranha::Range<const ns::range02 &>);
    REQUIRE(!piranha::InputRange<ns::range02>);
    REQUIRE(!piranha::InputRange<ns::range02 &>);
    REQUIRE(!piranha::InputRange<const ns::range02 &>);
    REQUIRE(!piranha::Range<ns::range03>);
    REQUIRE(!piranha::InputRange<ns::range03>);
    REQUIRE(!piranha::ForwardRange<ns::range03>);
    REQUIRE(!piranha::MutableForwardRange<ns::range03>);
    REQUIRE(piranha::Range<ns::range03 &>);
    REQUIRE(!piranha::Range<const ns::range03 &>);
    REQUIRE(piranha::InputRange<ns::range04 &>);
    REQUIRE(!piranha::ForwardRange<ns::range04 &>);
    REQUIRE(!piranha::Range<ns::norange00>);
    REQUIRE(!piranha::InputRange<ns::norange00>);
    REQUIRE(!piranha::ForwardRange<ns::norange00>);
    REQUIRE(!piranha::MutableForwardRange<ns::norange00>);
    REQUIRE(!piranha::Range<ns::norange00 &>);
    REQUIRE(!piranha::Range<const ns::norange00 &>);
    REQUIRE(!piranha::Range<ns::norange01>);
    REQUIRE(!piranha::Range<ns::norange01 &>);
    REQUIRE(!piranha::Range<const ns::norange01 &>);
    REQUIRE(!piranha::Range<ns::norange02>);
    REQUIRE(!piranha::Range<ns::norange02 &>);
    REQUIRE(!piranha::Range<const ns::norange02 &>);
    REQUIRE(!piranha::Range<ns::norange03>);
    REQUIRE(!piranha::Range<ns::norange03 &>);
    REQUIRE(!piranha::Range<const ns::norange03 &>);
#endif

    // A couple of runtime tests.
    std::vector<int> v_int_0 = {1, 2, 3};
    REQUIRE(piranha::begin(v_int_0) == v_int_0.begin());
    REQUIRE(piranha::end(v_int_0) == v_int_0.end());

    double arr_d[] = {4, 5, 6};
    REQUIRE(piranha::begin(arr_d) == &arr_d[0]);
    REQUIRE(piranha::end(arr_d) == &arr_d[0] + 3);
}
