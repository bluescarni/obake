// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <mp++/integer.hpp>
#include <mp++/rational.hpp>

#include <obake/config.hpp>
#include <obake/type_name.hpp>

#include "catch.hpp"

using namespace obake;

TEST_CASE("type_name")
{
    std::cout << type_name<std::string>() << '\n';
    std::cout << type_name<int>() << '\n';
    std::cout << type_name<const int>() << '\n';
    std::cout << type_name<const volatile int>() << '\n';
    std::cout << type_name<volatile int>() << '\n';
    std::cout << type_name<long double>() << '\n';
    std::cout << type_name<std::vector<std::vector<float>>>() << '\n';
    std::cout << type_name<mppp::integer<1>>() << '\n';
    std::cout << type_name<mppp::rational<2>>() << '\n';
    std::cout << type_name<mppp::rational<2> &>() << '\n';
    std::cout << type_name<mppp::rational<2> const>() << '\n';
    std::cout << type_name<mppp::rational<2> const &>() << '\n';
    std::cout << type_name<mppp::rational<2> *>() << '\n';
    std::cout << type_name<const mppp::rational<2> *>() << '\n';
    std::cout << type_name<const mppp::rational<2> *const>() << '\n';
    std::cout << type_name<const mppp::rational<2> *const &>() << '\n';
    std::cout << type_name<void>() << '\n';
    std::cout << type_name<void const>() << '\n';
    std::cout << type_name<void volatile>() << '\n';
    std::cout << type_name<void volatile const>() << '\n';
#if defined(OBAKE_HAVE_GCC_INT128)
    std::cout << type_name<__int128_t>() << '\n';
    std::cout << type_name<__int128_t *>() << '\n';
    std::cout << type_name<__int128_t const *>() << '\n';
    std::cout << type_name<__int128_t const *const>() << '\n';
    std::cout << type_name<__uint128_t>() << '\n';
    std::cout << type_name<__uint128_t *>() << '\n';
    std::cout << type_name<__uint128_t const *>() << '\n';
    std::cout << type_name<__uint128_t const *const>() << '\n';
    std::cout << type_name<const __int128_t>() << '\n';
    std::cout << type_name<const __uint128_t>() << '\n';
    std::cout << type_name<__int128_t &>() << '\n';
    std::cout << type_name<__uint128_t &>() << '\n';
    std::cout << type_name<__int128_t &&>() << '\n';
    std::cout << type_name<__uint128_t &&>() << '\n';
    std::cout << type_name<const __int128_t &>() << '\n';
    std::cout << type_name<const __uint128_t &>() << '\n';
    std::cout << type_name<std::vector<__int128_t>>() << '\n';
#endif

    // Couple of multithreaded tests.
    auto t_func = []() -> std::string {
        std::string tmp;
        for (auto i = 0; i < 100; ++i) {
            tmp += type_name<std::vector<std::vector<float>>>();
        }
        return tmp;
    };

    std::thread t1(t_func);
    std::thread t2(t_func);
    std::thread t3(t_func);
    std::thread t4(t_func);
    std::thread t5(t_func);
    std::thread t6(t_func);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
}
