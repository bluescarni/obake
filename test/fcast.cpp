// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <type_traits>
#include <utility>

#include <obake/detail/fcast.hpp>

#include "catch.hpp"

using namespace obake;

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

TEST_CASE("fcast_test")
{
    REQUIRE(std::is_same_v<decltype(detail::fcast(4)), int &&>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(foo{})), foo &&>);

    REQUIRE(std::is_same_v<decltype(detail::fcast(bar1())), int &&>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(bar2())), foo &&>);

    REQUIRE(std::is_same_v<decltype(detail::fcast(bar3())), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(bar4())), const foo &>);

    REQUIRE(std::is_same_v<decltype(detail::fcast(bar5())), int &&>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(bar6())), foo &&>);

    REQUIRE(std::is_same_v<decltype(detail::fcast(bar7())), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(bar8())), const foo &>);

    int n1 = 0;
    foo f1;
    auto &n1ref = n1;
    auto &f1ref = f1;
    const auto &n1cref = n1;
    const auto &f1cref = f1;
    auto &&n1rref = std::move(n1);
    auto &&f1rref = std::move(f1);

    REQUIRE(std::is_same_v<decltype(detail::fcast(n1)), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(f1)), const foo &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(n1ref)), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(f1ref)), const foo &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(n1cref)), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(f1cref)), const foo &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(n1rref)), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(f1rref)), const foo &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(std::move(n1))), int &&>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(std::move(f1))), foo &&>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(static_cast<int &&>(n1))), int &&>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(static_cast<foo &&>(f1))), foo &&>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(static_cast<const int &&>(n1))), const int &>);
    REQUIRE(std::is_same_v<decltype(detail::fcast(static_cast<const foo &&>(f1))), const foo &>);
}
