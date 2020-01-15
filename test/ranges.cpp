// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <array>
#include <initializer_list>
#include <iterator>
#include <list>
#include <set>
#include <unordered_set>
#include <vector>

#include <obake/ranges.hpp>

#include "catch.hpp"

// Test constexpr capabilities.
constexpr std::array<int, 3> aint{{1, 2, 3}};

template <bool>
struct foo {
};

// NOTE: begin/end are not constexpr in MSVC 2015.
#if !defined(_MSC_VER) || _MSC_VER >= 1910

[[maybe_unused]] foo<obake::begin(aint) != obake::end(aint)> f;

#endif

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
    REQUIRE(!obake::is_range_v<void>);
    REQUIRE(!obake::is_input_range_v<void>);
    REQUIRE(!obake::is_forward_range_v<void>);
    REQUIRE(!obake::is_mutable_forward_range_v<void>);
    REQUIRE(obake::is_range_v<std::vector<int>>);
    REQUIRE(obake::is_range_v<const std::vector<int>>);
    REQUIRE(obake::is_range_v<std::vector<int> &>);
    REQUIRE(obake::is_range_v<std::vector<int> &&>);
    REQUIRE(obake::is_range_v<const std::vector<int> &>);
    REQUIRE(obake::is_range_v<int(&)[3]>);
    REQUIRE(obake::is_range_v<ns::range00>);
    REQUIRE(obake::is_range_v<ns::range00 &>);
    REQUIRE(obake::is_range_v<const ns::range00 &>);
    REQUIRE(obake::is_mutable_forward_range_v<std::vector<int> &>);
    REQUIRE(!obake::is_mutable_forward_range_v<const std::vector<int> &>);
    REQUIRE((!obake::is_mutable_forward_range_v<std::initializer_list<int> &&>));
    REQUIRE(obake::is_mutable_forward_range_v<std::list<int> &>);
    REQUIRE(!obake::is_mutable_forward_range_v<const std::list<int> &>);
    REQUIRE(obake::is_mutable_forward_range_v<int(&)[3]>);
    REQUIRE(!obake::is_mutable_forward_range_v<const int(&)[3]>);
    REQUIRE(obake::is_input_range_v<ns::range00>);
    REQUIRE(obake::is_input_range_v<ns::range00 &>);
    REQUIRE(obake::is_input_range_v<const ns::range00 &>);
    REQUIRE(obake::is_forward_range_v<ns::range00>);
    REQUIRE(obake::is_forward_range_v<ns::range00 &>);
    REQUIRE(obake::is_forward_range_v<const ns::range00 &>);
    REQUIRE(obake::is_range_v<ns::range01>);
    REQUIRE(obake::is_range_v<ns::range01 &>);
    REQUIRE(obake::is_range_v<const ns::range01 &>);
    REQUIRE(obake::is_range_v<ns::range02>);
    REQUIRE(obake::is_range_v<ns::range02 &>);
    REQUIRE(obake::is_range_v<const ns::range02 &>);
    REQUIRE(!obake::is_input_range_v<ns::range02>);
    REQUIRE(!obake::is_input_range_v<ns::range02 &>);
    REQUIRE(!obake::is_input_range_v<const ns::range02 &>);
    REQUIRE(!obake::is_range_v<ns::range03>);
    REQUIRE(!obake::is_input_range_v<ns::range03>);
    REQUIRE(!obake::is_forward_range_v<ns::range03>);
    REQUIRE(!obake::is_mutable_forward_range_v<ns::range03>);
    REQUIRE(obake::is_range_v<ns::range03 &>);
    REQUIRE(!obake::is_range_v<const ns::range03 &>);
    REQUIRE(obake::is_input_range_v<ns::range04 &>);
    REQUIRE(!obake::is_forward_range_v<ns::range04 &>);
    REQUIRE(!obake::is_range_v<ns::norange00>);
    REQUIRE(!obake::is_input_range_v<ns::norange00>);
    REQUIRE(!obake::is_forward_range_v<ns::norange00>);
    REQUIRE(!obake::is_mutable_forward_range_v<ns::norange00>);
    REQUIRE(!obake::is_range_v<ns::norange00 &>);
    REQUIRE(!obake::is_range_v<const ns::norange00 &>);
    REQUIRE(!obake::is_range_v<ns::norange01>);
    REQUIRE(!obake::is_range_v<ns::norange01 &>);
    REQUIRE(!obake::is_range_v<const ns::norange01 &>);
    REQUIRE(!obake::is_range_v<ns::norange02>);
    REQUIRE(!obake::is_range_v<ns::norange02 &>);
    REQUIRE(!obake::is_range_v<const ns::norange02 &>);
    REQUIRE(!obake::is_range_v<ns::norange03>);
    REQUIRE(!obake::is_range_v<ns::norange03 &>);
    REQUIRE(!obake::is_range_v<const ns::norange03 &>);
    REQUIRE(obake::is_bidirectional_range_v<std::vector<int>>);
    REQUIRE(obake::is_bidirectional_range_v<std::vector<int> &>);
    REQUIRE(obake::is_bidirectional_range_v<const std::vector<int> &>);
    REQUIRE(obake::is_bidirectional_range_v<const std::vector<int>>);
    REQUIRE(obake::is_bidirectional_range_v<std::list<int>>);
    REQUIRE(obake::is_bidirectional_range_v<int(&)[3]>);
    REQUIRE(!obake::is_bidirectional_range_v<ns::range04>);
    REQUIRE(!obake::is_bidirectional_range_v<ns::range04 &>);
    REQUIRE(!obake::is_bidirectional_range_v<const ns::range04 &>);
    REQUIRE(!obake::is_bidirectional_range_v<const ns::range04>);
    REQUIRE(obake::is_random_access_range_v<std::vector<int>>);
    REQUIRE(!obake::is_random_access_range_v<std::list<int>>);
    REQUIRE(obake::is_random_access_range_v<std::vector<int> &>);
    REQUIRE(obake::is_random_access_range_v<const std::vector<int> &>);
    REQUIRE(obake::is_random_access_range_v<const std::vector<int>>);
    REQUIRE(obake::is_random_access_range_v<int(&)[3]>);
    REQUIRE(!obake::is_random_access_range_v<ns::range04>);
    REQUIRE(!obake::is_random_access_range_v<ns::range04 &>);
    REQUIRE(!obake::is_random_access_range_v<const ns::range04 &>);
    REQUIRE(!obake::is_random_access_range_v<const ns::range04>);
    REQUIRE(!obake::is_random_access_range_v<std::set<int>>);
    REQUIRE(!obake::is_random_access_range_v<std::unordered_set<int>>);

#if defined(OBAKE_HAVE_CONCEPTS)
    REQUIRE(!obake::Range<void>);
    REQUIRE(!obake::InputRange<void>);
    REQUIRE(!obake::ForwardRange<void>);
    REQUIRE(!obake::MutableForwardRange<void>);
    REQUIRE(obake::Range<std::vector<int>>);
    REQUIRE(obake::Range<const std::vector<int>>);
    REQUIRE(obake::Range<std::vector<int> &>);
    REQUIRE(obake::Range<std::vector<int> &&>);
    REQUIRE(obake::Range<const std::vector<int> &>);
    REQUIRE(obake::Range<int(&)[3]>);
    REQUIRE(obake::Range<ns::range00>);
    REQUIRE(obake::Range<ns::range00 &>);
    REQUIRE(obake::Range<const ns::range00 &>);
    REQUIRE(obake::MutableForwardRange<std::vector<int> &>);
    REQUIRE(!obake::MutableForwardRange<const std::vector<int> &>);
    REQUIRE((!obake::MutableForwardRange<std::initializer_list<int> &&>));
    REQUIRE(obake::MutableForwardRange<std::list<int> &>);
    REQUIRE(!obake::MutableForwardRange<const std::list<int> &>);
    REQUIRE(obake::MutableForwardRange<int(&)[3]>);
    REQUIRE(!obake::MutableForwardRange<const int(&)[3]>);
    REQUIRE(obake::InputRange<ns::range00>);
    REQUIRE(obake::InputRange<ns::range00 &>);
    REQUIRE(obake::InputRange<const ns::range00 &>);
    REQUIRE(obake::ForwardRange<ns::range00>);
    REQUIRE(obake::ForwardRange<ns::range00 &>);
    REQUIRE(obake::ForwardRange<const ns::range00 &>);
    REQUIRE(obake::Range<ns::range01>);
    REQUIRE(obake::Range<ns::range01 &>);
    REQUIRE(obake::Range<const ns::range01 &>);
    REQUIRE(obake::Range<ns::range02>);
    REQUIRE(obake::Range<ns::range02 &>);
    REQUIRE(obake::Range<const ns::range02 &>);
    REQUIRE(!obake::InputRange<ns::range02>);
    REQUIRE(!obake::InputRange<ns::range02 &>);
    REQUIRE(!obake::InputRange<const ns::range02 &>);
    REQUIRE(!obake::Range<ns::range03>);
    REQUIRE(!obake::InputRange<ns::range03>);
    REQUIRE(!obake::ForwardRange<ns::range03>);
    REQUIRE(!obake::MutableForwardRange<ns::range03>);
    REQUIRE(obake::Range<ns::range03 &>);
    REQUIRE(!obake::Range<const ns::range03 &>);
    REQUIRE(obake::InputRange<ns::range04 &>);
    REQUIRE(!obake::ForwardRange<ns::range04 &>);
    REQUIRE(!obake::Range<ns::norange00>);
    REQUIRE(!obake::InputRange<ns::norange00>);
    REQUIRE(!obake::ForwardRange<ns::norange00>);
    REQUIRE(!obake::MutableForwardRange<ns::norange00>);
    REQUIRE(!obake::Range<ns::norange00 &>);
    REQUIRE(!obake::Range<const ns::norange00 &>);
    REQUIRE(!obake::Range<ns::norange01>);
    REQUIRE(!obake::Range<ns::norange01 &>);
    REQUIRE(!obake::Range<const ns::norange01 &>);
    REQUIRE(!obake::Range<ns::norange02>);
    REQUIRE(!obake::Range<ns::norange02 &>);
    REQUIRE(!obake::Range<const ns::norange02 &>);
    REQUIRE(!obake::Range<ns::norange03>);
    REQUIRE(!obake::Range<ns::norange03 &>);
    REQUIRE(!obake::Range<const ns::norange03 &>);
    REQUIRE(obake::BidirectionalRange<std::vector<int>>);
    REQUIRE(obake::BidirectionalRange<std::list<int>>);
    REQUIRE(obake::BidirectionalRange<std::vector<int> &>);
    REQUIRE(obake::BidirectionalRange<const std::vector<int> &>);
    REQUIRE(obake::BidirectionalRange<const std::vector<int>>);
    REQUIRE(obake::BidirectionalRange<int(&)[3]>);
    REQUIRE(!obake::BidirectionalRange<ns::range04>);
    REQUIRE(!obake::BidirectionalRange<ns::range04 &>);
    REQUIRE(!obake::BidirectionalRange<const ns::range04 &>);
    REQUIRE(!obake::BidirectionalRange<const ns::range04>);
    REQUIRE(obake::RandomAccessRange<std::vector<int>>);
    REQUIRE(!obake::RandomAccessRange<std::list<int>>);
    REQUIRE(obake::RandomAccessRange<std::vector<int> &>);
    REQUIRE(obake::RandomAccessRange<const std::vector<int> &>);
    REQUIRE(obake::RandomAccessRange<const std::vector<int>>);
    REQUIRE(obake::RandomAccessRange<int(&)[3]>);
    REQUIRE(!obake::RandomAccessRange<ns::range04>);
    REQUIRE(!obake::RandomAccessRange<ns::range04 &>);
    REQUIRE(!obake::RandomAccessRange<const ns::range04 &>);
    REQUIRE(!obake::RandomAccessRange<const ns::range04>);
    REQUIRE(!obake::RandomAccessRange<std::set<int>>);
    REQUIRE(!obake::RandomAccessRange<std::unordered_set<int>>);
#endif

    // A couple of runtime tests.
    std::vector<int> v_int_0 = {1, 2, 3};
    REQUIRE(obake::begin(v_int_0) == v_int_0.begin());
    REQUIRE(obake::end(v_int_0) == v_int_0.end());

    double arr_d[] = {4, 5, 6};
    REQUIRE(obake::begin(arr_d) == &arr_d[0]);
    REQUIRE(obake::end(arr_d) == &arr_d[0] + 3);
}

#if !defined(_MSC_VER) || _MSC_VER >= 1910

// Verify constexpr capabilities.
constexpr std::array<int, 3> carr{};
[[maybe_unused]] constexpr auto carr_range = obake::detail::make_range(carr.begin(), carr.end());

#endif

TEST_CASE("make_range_test")
{
    std::vector<int> v{1, 2, 3};

    auto r = obake::detail::make_range(v.begin(), v.end());
    REQUIRE(r.b == v.begin());
    REQUIRE(r.e == v.end());
    REQUIRE(std::is_same_v<decltype(r.b), std::vector<int>::iterator>);
    REQUIRE(obake::is_range_v<decltype(r)>);
    REQUIRE(obake::is_input_range_v<decltype(r)>);
    REQUIRE(obake::is_forward_range_v<decltype(r)>);
    REQUIRE(obake::is_mutable_forward_range_v<decltype(r)>);

    auto rc = obake::detail::make_range(v.cbegin(), v.cend());
    REQUIRE(rc.b == v.cbegin());
    REQUIRE(rc.e == v.cend());
    REQUIRE(std::is_same_v<decltype(rc.b), std::vector<int>::const_iterator>);
    REQUIRE(obake::is_range_v<decltype(rc)>);
    REQUIRE(obake::is_input_range_v<decltype(rc)>);
    REQUIRE(obake::is_forward_range_v<decltype(rc)>);
    REQUIRE(!obake::is_mutable_forward_range_v<decltype(rc)>);
}
