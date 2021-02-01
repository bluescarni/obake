// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)::polynomials
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_POISSON_SERIES_D_PACKED_TRIG_MONOMIAL_HPP
#define OBAKE_POISSON_SERIES_D_PACKED_TRIG_MONOMIAL_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <stdexcept>

#include <boost/container/container_fwd.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/version.hpp>

// NOTE: the header for hash_combine changed in version 1.67.
#if (BOOST_VERSION / 100000 > 1) || (BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 >= 67)

#include <boost/container_hash/hash.hpp>

#else

#include <boost/functional/hash.hpp>

#endif

#include <fmt/format.h>

#include <obake/config.hpp>
#include <obake/exceptions.hpp>
#include <obake/kpack.hpp>
#include <obake/math/safe_cast.hpp>
#include <obake/polynomials/d_packed_monomial.hpp>
#include <obake/ranges.hpp>
#include <obake/s11n.hpp>
#include <obake/symbols.hpp>
#include <obake/type_traits.hpp>

namespace obake
{

namespace poisson_series
{

// Max psize for d_packed_trig_monomial.
template <kpackable T>
requires(is_signed_v<T> == true) inline constexpr unsigned dptm_max_psize = ::obake::detail::kpack_max_size<T>();

// Dynamic packed trigonometric monomial.
template <kpackable T, unsigned PSize>
    requires(is_signed_v<T> == true) && (PSize > 0u) && (PSize <= dptm_max_psize<T>)class d_packed_trig_monomial
{
    friend class ::boost::serialization::access;

public:
    // The container type.
    using container_t = ::boost::container::small_vector<T, 1>;

private:
    container_t m_container;
    // true -> cosine, false -> sine.
    bool m_type = true;

public:
    // Alias for PSize
    static constexpr unsigned psize = PSize;

    // Alias for T.
    using value_type = T;

    // Default constructor.
    d_packed_trig_monomial() = default;

    // Constructor from symbol set.
    explicit d_packed_trig_monomial(const symbol_set &ss, bool type = true)
        : m_container(::obake::safe_cast<typename container_t::size_type>(
            polynomials::detail::dpm_n_expos_to_vsize<d_packed_trig_monomial>(ss.size()))),
          m_type(type)
    {
    }

    // Constructor from input iterator and size.
    template <typename It>
    requires InputIterator<It> &&
        SafelyCastable<typename ::std::iterator_traits<It>::reference, T> explicit d_packed_trig_monomial(
            It it, ::std::size_t n, bool type = true)
        : m_container(::obake::safe_cast<typename container_t::size_type>(
                          polynomials::detail::dpm_n_expos_to_vsize<d_packed_trig_monomial>(n)),
                      // NOTE: avoid value-init of the elements, as we will
                      // be setting all of them to some value in the loop below.
                      ::boost::container::default_init_t{}),
          m_type(type)
    {
        ::std::size_t counter = 0;
        bool first_nz_found = false;
        for (auto &out : m_container) {
            kpacker<T> kp(psize);

            // Keep packing until we get to psize or we have
            // exhausted the input values.
            for (auto j = 0u; j < psize && counter < n; ++j, ++counter, ++it) {
                const auto tmp = ::obake::safe_cast<T>(*it);

                if (obake_unlikely(!first_nz_found && tmp < 0)) {
                    // This is the first nonzero exponent
                    // and it is negative, thus the canonical
                    // form of the monomial is violated.
                    using namespace ::fmt::literals;
                    obake_throw(::std::invalid_argument,
                                "Cannot construct a trigonometric monomial whose first nonzero "
                                "exponent ({}) is negative"_format(tmp));
                }

                // Update first_nz_found.
                first_nz_found = (first_nz_found || tmp != 0);

                kp << tmp;
            }

            out = kp.get();
        }
    }

private:
    struct input_it_ctor_tag {
    };
    // Implementation of the ctor from input iterators.
    // NOTE: a possible optimisation here is to detect
    // random-access iterators and delegate to the
    // ctor from input iterator and size.
    template <typename It>
    explicit d_packed_trig_monomial(input_it_ctor_tag, It b, It e, bool type) : m_type(type)
    {
        bool first_nz_found = false;

        while (b != e) {
            kpacker<T> kp(psize);

            for (auto j = 0u; j < psize && b != e; ++j, ++b) {
                const auto tmp = ::obake::safe_cast<T>(*b);

                if (obake_unlikely(!first_nz_found && tmp < 0)) {
                    // This is the first nonzero exponent
                    // and it is negative, thus the canonical
                    // form of the monomial is violated.
                    using namespace ::fmt::literals;
                    obake_throw(::std::invalid_argument,
                                "Cannot construct a trigonometric monomial whose first nonzero "
                                "exponent ({}) is negative"_format(tmp));
                }

                // Update first_nz_found.
                first_nz_found = (first_nz_found || tmp != 0);

                kp << tmp;
            }

            m_container.push_back(kp.get());
        }
    }

public:
    // Ctor from a pair of input iterators.
    template <typename It>
    requires InputIterator<It> &&
        SafelyCastable<typename ::std::iterator_traits<It>::reference, T> explicit d_packed_trig_monomial(It b, It e,
                                                                                                          bool type
                                                                                                          = true)
        : d_packed_trig_monomial(input_it_ctor_tag{}, b, e, type)
    {
    }

    // Ctor from input range.
    template <typename Range>
    requires InputRange<Range> &&SafelyCastable<typename ::std::iterator_traits<range_begin_t<Range>>::reference,
                                                T> explicit d_packed_trig_monomial(Range &&r, bool type = true)
        : d_packed_trig_monomial(input_it_ctor_tag{}, ::obake::begin(::std::forward<Range>(r)),
                                 ::obake::end(::std::forward<Range>(r)), type)
    {
    }

    // Ctor from init list.
    template <typename U>
    requires SafelyCastable<const U &, T> explicit d_packed_trig_monomial(::std::initializer_list<U> l)
        : d_packed_trig_monomial(input_it_ctor_tag{}, l.begin(), l.end(), true)
    {
    }

    container_t &_container()
    {
        return m_container;
    }
    const container_t &_container() const
    {
        return m_container;
    }

    bool &_type()
    {
        return m_type;
    }
    const bool &type() const
    {
        return m_type;
    }

private:
    // Serialisation.
    // NOTE: when we improve the s11n experience,
    // we will probably want to verify
    // canonical form when loading from non-binary
    // archives.
    template <class Archive>
    void save(Archive &ar, unsigned) const
    {
        ar << m_container.size();

        for (const auto &n : m_container) {
            ar << n;
        }

        ar << m_type;
    }
    template <class Archive>
    void load(Archive &ar, unsigned)
    {
        decltype(m_container.size()) size;
        ar >> size;
        m_container.resize(size);

        for (auto &n : m_container) {
            ar >> n;
        }

        ar >> m_type;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

// Default PSize for d_packed_trig_monomial.
inline constexpr unsigned dptm_default_psize =
#if defined(OBAKE_PACKABLE_INT64)
    8
#else
    4
#endif
    ;

// Default type for the exponents.
using dptm_default_t =
#if defined(OBAKE_PACKABLE_INT64)
    ::std::int64_t
#else
    ::std::int32_t
#endif
    ;

// Implementation of key_is_zero(). A trigonometric monomial is zero
// if it is a sine and all the exponents are zero.
template <typename T, unsigned PSize>
inline bool key_is_zero(const d_packed_trig_monomial<T, PSize> &d, const symbol_set &)
{
    return d.type() == false
           && ::std::all_of(d._container().cbegin(), d._container().cend(), [](const T &n) { return n == T(0); });
}

// Implementation of key_is_one(). A trigonometric monomial is one if it is a cosine
// and all its exponents are zero.
template <typename T, unsigned PSize>
inline bool key_is_one(const d_packed_trig_monomial<T, PSize> &d, const symbol_set &)
{
    return d.type() == true
           && ::std::all_of(d._container().cbegin(), d._container().cend(), [](const T &n) { return n == T(0); });
}

// Comparisons.
template <typename T, unsigned PSize>
inline bool operator==(const d_packed_trig_monomial<T, PSize> &d1, const d_packed_trig_monomial<T, PSize> &d2)
{
    return d1.type() == d2.type() && d1._container() == d2._container();
}

template <typename T, unsigned PSize>
inline bool operator!=(const d_packed_trig_monomial<T, PSize> &d1, const d_packed_trig_monomial<T, PSize> &d2)
{
    return !(d1 == d2);
}

// Hash implementation.
// NOTE: this is not homomorphic at this time,
// and it is not clear if that is needed at all.
// A homomorphic implementation would ignore the
// type of the monomial and just add the exponents,
// at the price of a possible
// increase in collisions.
// NOTE: perhaps the extra mixing via hash_combine()
// is not really necessary, but on the other hand
// performance in poisson_series should not be really
// bottlenecked by this. Revisit when we have more data.
// Also, once we have benchmarks, we should also
// investigate the absl hashing performance.
template <typename T, unsigned PSize>
inline ::std::size_t hash(const d_packed_trig_monomial<T, PSize> &d)
{
    auto ret = static_cast<::std::size_t>(d.type());
    for (const auto &n : d._container()) {
        ::boost::hash_combine(ret, n);
    }
    return ret;
}

} // namespace poisson_series

// Lift to the obake namespace.
template <typename T, unsigned PSize>
using d_packed_trig_monomial = poisson_series::d_packed_trig_monomial<T, PSize>;

} // namespace obake

namespace boost::serialization
{

// Disable tracking for d_packed_trig_monomial.
template <typename T, unsigned PSize>
struct tracking_level<::obake::d_packed_trig_monomial<T, PSize>>
    : ::obake::detail::s11n_no_tracking<::obake::d_packed_trig_monomial<T, PSize>> {
};

} // namespace boost::serialization

#endif
