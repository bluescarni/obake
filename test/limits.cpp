// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <tuple>

#include <obake/config.hpp>
#include <obake/detail/limits.hpp>
#include <obake/detail/to_string.hpp>
#include <obake/detail/tuple_for_each.hpp>
#include <obake/type_name.hpp>

#include "catch.hpp"

using int_types = std::tuple<char, signed char, unsigned char, short, unsigned short, int, unsigned, long,
                             unsigned long, long long, unsigned long long
#if defined(OBAKE_HAVE_GCC_INT128)
                             ,
                             __int128_t, __uint128_t
#endif
                             >;

using namespace obake;

TEST_CASE("integer_limits_test")
{
    detail::tuple_for_each(int_types{}, [](auto n) {
        using int_t = decltype(n);

        std::cout << "Printing limits for the type '" + type_name<int_t>() << "'\n";
        std::cout << "Min     : " << detail::to_string(+detail::limits_min<int_t>) << '\n';
        std::cout << "Max     : " << detail::to_string(+detail::limits_max<int_t>) << '\n';
        std::cout << "Digits  : " << detail::limits_digits<int_t> << '\n';
        std::cout << "Digits10: " << detail::limits_digits10<int_t> << '\n';

        std::cout << "---------\n\n";
    });
}