// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <tuple>

#include <piranha/config.hpp>
#include <piranha/detail/limits.hpp>
#include <piranha/detail/to_string.hpp>
#include <piranha/detail/tuple_for_each.hpp>
#include <piranha/type_name.hpp>

#include "catch.hpp"

using int_types = std::tuple<char, signed char, unsigned char, short, unsigned short, int, unsigned, long,
                             unsigned long, long long, unsigned long long
#if defined(PIRANHA_HAVE_GCC_INT128)
                             ,
                             __int128_t, __uint128_t
#endif
                             >;

using namespace piranha;

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