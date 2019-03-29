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
#include <vector>

// Test constexpr capabilities.
constexpr std::array aint{1, 2, 3};

template <bool>
struct foo {
};

[[maybe_unused]] foo<piranha::begin(aint) != piranha::end(aint)> f;

namespace ns
{

struct range00 {
};

int *begin(const range00 &);
int *end(const range00 &);

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
    REQUIRE(piranha::is_range_v<std::vector<int>>);
    REQUIRE(piranha::is_range_v<const std::vector<int>>);
    REQUIRE(piranha::is_range_v<std::vector<int> &>);
    REQUIRE(piranha::is_range_v<std::vector<int> &&>);
    REQUIRE(piranha::is_range_v<const std::vector<int> &>);
    REQUIRE(piranha::is_range_v<int(&)[3]>);
    REQUIRE(piranha::is_range_v<ns::range00>);
    REQUIRE(piranha::is_range_v<ns::range00 &>);
    REQUIRE(piranha::is_range_v<const ns::range00 &>);
    REQUIRE(!piranha::is_range_v<ns::norange00>);
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
    REQUIRE(piranha::Range<std::vector<int>>);
    REQUIRE(piranha::Range<const std::vector<int>>);
    REQUIRE(piranha::Range<std::vector<int> &>);
    REQUIRE(piranha::Range<std::vector<int> &&>);
    REQUIRE(piranha::Range<const std::vector<int> &>);
    REQUIRE(piranha::Range<int(&)[3]>);
    REQUIRE(piranha::Range<ns::range00>);
    REQUIRE(piranha::Range<ns::range00 &>);
    REQUIRE(piranha::Range<const ns::range00 &>);
    REQUIRE(!piranha::Range<ns::norange00>);
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

    std::vector<int> v_int_0 = {1, 2, 3};
    REQUIRE(piranha::begin(v_int_0) == v_int_0.begin());
}
