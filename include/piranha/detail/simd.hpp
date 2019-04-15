// Copyright 2019 Francesco Biscani (bluescarni@gmail.com)
//
// This file is part of the piranha library.
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PIRANHA_DETAIL_SIMD_HPP
#define PIRANHA_DETAIL_SIMD_HPP

#if !defined(PIRANHA_DISABLE_SIMD)

#if defined(__AVX2__)

// Start checking for AVX2, this macro is defined
// essentially everywhere:
// https://stackoverflow.com/questions/28939652/how-to-detect-sse-sse2-avx-avx2-avx-512-avx-128-fma-kcvi-availability-at-compile
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=vs-2019
#define PIRANHA_HAVE_AVX2

#else

// For SSE2, we have to special-case MSVC:
// https://stackoverflow.com/questions/18563978/detect-the-availability-of-sse-sse2-instruction-set-in-visual-studio
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=vs-2019

#if defined(_MSC_VER)

#if _M_IX86_FP == 2

#define PIRANHA_HAVE_SSE2

#endif

#else

#if defined(__SSE2__)

#define PIRANHA_HAVE_SSE2

#endif

#endif

#endif

#endif

#if defined(PIRANHA_HAVE_AVX2) || defined(PIRANHA_HAVE_SSE2)

#if defined(_MSC_VER)

#include <intrin.h>

#else

#include <x86intrin.h>

#endif

#endif

namespace piranha::detail
{

} // namespace piranha::detail

#endif
