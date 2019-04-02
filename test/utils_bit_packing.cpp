// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <piranha/utils/bit_packing.hpp>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <stdexcept>
#include <tuple>

#include <piranha/config.hpp>
#include <piranha/detail/tuple_for_each.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

using int_types = std::tuple<int, unsigned, long, unsigned long, long long, unsigned long long
#if defined(PIRANHA_HAVE_GCC_INT128)
                             ,
                             __int128_t, __uint128_t
#endif
                             >;

TEST_CASE("bit_packer")
{
    detail::tuple_for_each(int_types{}, [](const auto &n) {
        using int_t = remove_cvref_t<decltype(n)>;
        using bp_t = bit_packer<int_t>;
        using value_t = typename bp_t::value_type;

        using Catch::Matchers::Contains;

        // Start with an empty packer.
        bit_packer<int_t> bp0(0);
        REQUIRE(bp0.get() == value_t(0));

        // Check that adding a value to the packer throws.
        REQUIRE_THROWS_WITH(
            bp0 << int_t{0},
            Contains("Cannot push any more values to this bit packer: the number of "
                     "values already pushed to the packer is equal to the size used for construction (0)"));
        REQUIRE_THROWS_AS(bp0 << int_t{0}, std::out_of_range);
    });
}
