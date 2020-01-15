// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <functional>
#include <mutex>
#include <string>
#include <tuple>

#include <obake/series.hpp>

namespace obake
{
namespace detail
{

// Implementation of the default streaming for a single term.
void series_stream_single_term(::std::string &ret, ::std::string &str_cf, const ::std::string &str_key, bool tex_mode)
{
    // Detect unitary cf/key.
    const bool cf_is_one = (str_cf == "1"), cf_is_minus_one = (str_cf == "-1"), key_is_one = (str_key == "1");

    if (cf_is_one && !key_is_one) {
        // Suppress the coefficient if it is "1"
        // and the key is not "1".
        str_cf.clear();
    } else if (cf_is_minus_one && !key_is_one) {
        // Turn the coefficient into a minus sign
        // if it is -1 and the key is not "1".
        str_cf = '-';
    } else if (key_is_one && str_cf.size() > 2u && str_cf.front() == '(' && str_cf.back() == ')') {
        // If the key is unitary, and the coefficient
        // consists of something enclosed in round brackets,
        // then remove them.
        str_cf = ::std::string(str_cf.begin() + 1, str_cf.end() - 1);
    }

    // Append the (possibly-transformed) coefficient.
    ret += str_cf;
    if (!cf_is_one && !cf_is_minus_one && !key_is_one && !tex_mode) {
        // If the abs(coefficient) is not unitary,
        // the key is also not unitary, and we are not
        // in Tex mode, then we need the
        // multiplication sign.
        ret += '*';
    }

    // Append the key, if it is not unitary.
    if (!key_is_one) {
        ret += str_key;
    }
}

} // namespace detail

namespace customisation::internal
{

// On-demand instantiation of the global
// objects used to implement the pow map.
::std::tuple<series_pow_map_t &, ::std::mutex &> get_series_pow_map()
{
    static series_pow_map_t retval;
    static ::std::mutex mutex;
    return ::std::make_tuple(::std::ref(retval), ::std::ref(mutex));
}

void clear_series_pow_map()
{
    // Fetch the global data.
    auto [map, mutex] = internal::get_series_pow_map();

    // Lock down before accessing the cache.
    ::std::lock_guard<::std::mutex> lock(mutex);

    map.clear();
}

} // namespace customisation::internal

} // namespace obake
