// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP
#define PIRANHA_POLYNOMIALS_POLYNOMIAL_HPP

#include <piranha/series.hpp>

namespace piranha
{

namespace polynomials
{

struct tag {
};

template <typename Cf, typename Key>
using polynomial = series<Cf, Key, tag>;

template <typename Cf, typename Key>
inline polynomial<Cf, Key> pow(const polynomial<Cf, Key> &, int)
{
    return polynomial<Cf, Key>{};
}

} // namespace polynomials

template <typename Cf, typename Key>
using polynomial = polynomials::polynomial<Cf, Key>;

} // namespace piranha

#endif
