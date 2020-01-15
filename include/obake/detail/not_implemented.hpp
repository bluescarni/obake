// Copyright 2019-2020 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the obake library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OBAKE_DETAIL_NOT_IMPLEMENTED_HPP
#define OBAKE_DETAIL_NOT_IMPLEMENTED_HPP

namespace obake::customisation
{

struct not_implemented_t {
};

inline constexpr auto not_implemented = not_implemented_t{};

} // namespace obake::customisation

#endif
