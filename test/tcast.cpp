// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/detail/tcast.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <type_traits>
#include <utility>

using namespace piranha;

struct foo {
};

int bar1();
foo bar2();

int &bar3();
foo &bar4();

int &&bar5();
foo &&bar6();

const int &&bar7();
const foo &&bar8();

TEST_CASE("tcast_test")
{
    REQUIRE(std::is_same_v<decltype(detail::tcast(4)), int &&>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(foo{})), foo &&>);

    REQUIRE(std::is_same_v<decltype(detail::tcast(bar1())), int &&>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(bar2())), foo &&>);

    REQUIRE(std::is_same_v<decltype(detail::tcast(bar3())), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(bar4())), const foo &>);

    REQUIRE(std::is_same_v<decltype(detail::tcast(bar5())), int &&>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(bar6())), foo &&>);

    REQUIRE(std::is_same_v<decltype(detail::tcast(bar7())), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(bar8())), const foo &>);

    int n1 = 0;
    foo f1;
    auto &n1ref = n1;
    auto &f1ref = f1;
    const auto &n1cref = n1;
    const auto &f1cref = f1;
    auto &&n1rref = std::move(n1);
    auto &&f1rref = std::move(f1);

    REQUIRE(std::is_same_v<decltype(detail::tcast(n1)), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(f1)), const foo &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(n1ref)), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(f1ref)), const foo &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(n1cref)), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(f1cref)), const foo &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(n1rref)), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(f1rref)), const foo &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(std::move(n1))), int &&>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(std::move(f1))), foo &&>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(static_cast<int &&>(n1))), int &&>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(static_cast<foo &&>(f1))), foo &&>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(static_cast<const int &&>(n1))), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::tcast(static_cast<const foo &&>(f1))), const foo &>);
}
