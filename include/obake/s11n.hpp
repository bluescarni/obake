// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)::polynomials
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_S11N_HPP
#define OBAKE_S11N_HPP

#include <boost/config.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/static_assert.hpp>

#include <obake/config.hpp>

namespace obake::detail
{

// Small helper to disable tracking for a type T.
// NOTE: this is taken verbatim from the boost serialization macros.
template <typename T>
struct s11n_no_tracking {
    using tag = ::boost::mpl::integral_c_tag;
    using type = ::boost::mpl::int_<::boost::serialization::track_never>;
    BOOST_STATIC_CONSTANT(int, value = s11n_no_tracking::type::value);
    BOOST_STATIC_ASSERT((::boost::mpl::greater<::boost::serialization::implementation_level<T>,
                                               ::boost::mpl::int_<::boost::serialization::primitive_type>>::value));
};

// Static init.
template <typename T>
const int s11n_no_tracking<T>::value;

} // namespace obake::detail

#endif
