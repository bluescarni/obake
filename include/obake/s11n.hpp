// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)::polynomials
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_S11N_HPP
#define OBAKE_S11N_HPP

#include <cstdint>

#include <boost/config.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/static_assert.hpp>

#include <obake/config.hpp>

#if defined(OBAKE_HAVE_GCC_INT128)

// Implement serialisation for 128-bit integrals.
namespace boost::serialization
{

template <class Archive>
inline void save(Archive &ar, const __uint128_t &n, unsigned)
{
    // NOTE: save first the lower and then the upper limb.
    ar << static_cast<::std::uint64_t>(n);
    ar << static_cast<::std::uint64_t>(n >> 64);
}

template <class Archive>
inline void load(Archive &ar, __uint128_t &n, unsigned)
{
    ::std::uint64_t tmp;

    // Load the lower limb.
    ar >> tmp;
    n = tmp;

    // Load the upper limb, add it.
    ar >> tmp;
    n += static_cast<__uint128_t>(tmp) << 64;
}

// For __int128_t, just delegate to the unsigned counterpart.
template <class Archive>
inline void save(Archive &ar, const __int128_t &n, unsigned)
{
    ar << static_cast<__uint128_t>(n);
}

template <class Archive>
inline void load(Archive &ar, __int128_t &n, unsigned)
{
    __uint128_t tmp;
    ar >> tmp;
    n = static_cast<__int128_t>(tmp);
}

// Ensure that 128-bit integers are considered primitive types,
// which also ensures that their address is never tracked.
BOOST_STATIC_ASSERT((mpl::equal_to<implementation_level<__uint128_t>, mpl::int_<primitive_type>>::value));
BOOST_STATIC_ASSERT((mpl::equal_to<implementation_level<__int128_t>, mpl::int_<primitive_type>>::value));

} // namespace boost::serialization

BOOST_SERIALIZATION_SPLIT_FREE(__uint128_t)
BOOST_SERIALIZATION_SPLIT_FREE(__int128_t)

#endif

namespace obake::detail
{

// Small helper to disable tracking for a type T.
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
