Changelog
=========

0.8.0 (unreleased)
------------------

New
~~~

- obake can now be built as a static library
  (`#146 <https://github.com/bluescarni/obake/pull/146>`__).

Changes
~~~~~~~

- obake now requires mp++ >= 0.27
  (`#149 <https://github.com/bluescarni/obake/pull/149>`__).
- Various internal cleanups as a consequence of the C++20 migration
  (`#140 <https://github.com/bluescarni/obake/pull/140>`__).

Fix
~~~

- Workaround for build error in MSVC
  (`#144 <https://github.com/bluescarni/obake/pull/144>`__).

0.7.0 (2021-06-01)
------------------

New
~~~

- Add convenience typedefs for the monomial types
  (`#134 <https://github.com/bluescarni/obake/pull/134>`__).
- The C++ standard requirement is now exported in the CMake
  config-file package installed by obake
  (`#128 <https://github.com/bluescarni/obake/pull/128>`__).
- Implement truncated power series
  (`#127 <https://github.com/bluescarni/obake/pull/127>`__).

Changes
~~~~~~~

- obake now requires mp++ >= 0.23
  (`#139 <https://github.com/bluescarni/obake/pull/139>`__).
- Continue moving code from the headers into the compiled
  part of the library
  (`#134 <https://github.com/bluescarni/obake/pull/134>`__).
- **BREAKING**: obake is now based on C++20, and the minimum
  CMake version has been bumped up to 3.12. As a consequence,
  obake now requires GCC >= 9, clang >= 11 and MSVC >= 2019
  (`#128 <https://github.com/bluescarni/obake/pull/128>`__).
- obake now depends on the `{fmt} <https://fmt.dev/latest/index.html>`__
  library
  (`#125 <https://github.com/bluescarni/obake/pull/125>`__).
- **BREAKING**: the implementation of ``packed_monomial`` and
  ``d_packed_monomial`` has changed: now when specifying the desired
  exponent type, only ``std::(u)int32_t`` and ``std::(u)int64_t``
  are supported, and using other integral types (e.g., ``long``,
  ``unsigned long long``, etc.) may lead to compile-time errors,
  depending on how ``std::(u)int32_t`` and ``std::(u)int64_t``
  are implemented on the target platform
  (`#125 <https://github.com/bluescarni/obake/pull/125>`__).
- **BREAKING**: ``d_packed_monomial`` is now specified in terms
  of the number of exponents to be packed in a single integer
  rather than the bit width assigned to each exponent
  (`#125 <https://github.com/bluescarni/obake/pull/125>`__).
- The Kronecker packing code has been rewritten to avoid
  division and remainder operations, leading to a
  substantial performance uplift
  (`#125 <https://github.com/bluescarni/obake/pull/125>`__).
- Adopt a flyweight pattern for the representation of
  symbol sets in series
  (`#124 <https://github.com/bluescarni/obake/pull/124>`__).
- When building on Windows with ``clang-cl``, obake
  now requires at least MSVC 2017. As a result, various
  limitations/workarounds originating from the need to support
  the MSVC 2015 standard library have been removed
  (`#122 <https://github.com/bluescarni/obake/pull/122>`__).

Fix
~~~

- Fix build with mp++ >= 0.23
  (`#139 <https://github.com/bluescarni/obake/pull/139>`__).
- Fix build with oneTBB
  (`#139 <https://github.com/bluescarni/obake/pull/139>`__).
- Fix a bug when streaming monomials in tex mode
  (`#132 <https://github.com/bluescarni/obake/pull/132>`__).

0.6.0 (2020-06-12)
------------------

Fix
~~~

- Fix a bug in polynomial integration
  (`#119 <https://github.com/bluescarni/obake/pull/119>`__).
- Implement a workaround for a compiler bug in GCC 7
  (`#117 <https://github.com/bluescarni/obake/pull/117>`__).

0.5.0 (2020-05-11)
------------------

New
~~~

- Parallelise the ``byte_size()`` implementation for series
  types
  (`#115 <https://github.com/bluescarni/obake/pull/115>`__).
- obake's header files are now visible in the project
  files created by the MSVC CMake generators
  (`#112 <https://github.com/bluescarni/obake/pull/112>`__).

Changes
~~~~~~~

- Various simplifications and improvements to the benchmarks
  (`#114 <https://github.com/bluescarni/obake/pull/114>`__).
- Update Catch to the latest version, 2.12.1
  (`#114 <https://github.com/bluescarni/obake/pull/114>`__).

Fix
~~~

- Fix build failures with GCC 10 in C++20 mode
  (`#116 <https://github.com/bluescarni/obake/pull/116>`__).
- Fix build issues with recent CMake versions
  (`#111 <https://github.com/bluescarni/obake/pull/111>`__).

0.4.0 (2020-02-12)
------------------

New
~~~

- Implement a ``filtered()`` primitive to return
  the filtered copy of a series
  (`#98 <https://github.com/bluescarni/obake/pull/98>`__).
- obake now respects the ``CMAKE_CXX_STANDARD``
  variable, if set by the user
  (`#92 <https://github.com/bluescarni/obake/pull/92>`__).
- Implement explicit truncation based on the
  partial degree
  (`#91 <https://github.com/bluescarni/obake/pull/91>`__).

Changes
~~~~~~~

- Update Catch to the latest version, 2.11.1
  (`#107 <https://github.com/bluescarni/obake/pull/107>`__).
- **BREAKING**: functions and type traits dealing with
  in-place arithmetic have been renamed
  (`#106 <https://github.com/bluescarni/obake/pull/106>`__).
- **BREAKING**: the ``filter()`` and ``truncate_degree()``
  functions now operate in-place
  (`#98 <https://github.com/bluescarni/obake/pull/98>`__).
- Various improvements to the overflow checking
  machinery for polynomial exponents
  (`#94 <https://github.com/bluescarni/obake/pull/94>`__).

Fix
~~~

- Fix a bug in the conversion operator for series
  (`#99 <https://github.com/bluescarni/obake/pull/99>`__).
- Fix an overflow detection bug in the dynamic packed
  monomial class
  (`#94 <https://github.com/bluescarni/obake/pull/94>`__).
- Many MSVC fixes/improvements, including support for
  the latest MSVC builds in C++20 mode with concepts
  (`#50 <https://github.com/bluescarni/obake/pull/50>`__).
  Many thanks to `7ofNine <https://github.com/7ofNine>`__!
- Various cleanups and build system fixes/improvements
  (`#103 <https://github.com/bluescarni/obake/pull/103>`__,
  `#100 <https://github.com/bluescarni/obake/pull/100>`__,
  `#99 <https://github.com/bluescarni/obake/pull/99>`__,
  `#98 <https://github.com/bluescarni/obake/pull/98>`__,
  `#97 <https://github.com/bluescarni/obake/pull/97>`__,
  `#96 <https://github.com/bluescarni/obake/pull/96>`__,
  `#93 <https://github.com/bluescarni/obake/pull/93>`__,
  `#92 <https://github.com/bluescarni/obake/pull/92>`__,
  `#90 <https://github.com/bluescarni/obake/pull/90>`__).

0.3 (2019-10-31)
----------------

New
~~~

- Implement a caching mechanism for the natural powers
  of series
  (`#84 <https://github.com/bluescarni/obake/pull/84>`__).
- Implement safe conversion between rationals
  and C++ integrals
  (`#84 <https://github.com/bluescarni/obake/pull/84>`__).

Changes
~~~~~~~

- Significant speedups (and small fixes) for the series
  in-place arithmetic operators
  (`#85 <https://github.com/bluescarni/obake/pull/85>`__).
- Speedups and simplifications in the dynamic packed monomial class
  (`#83 <https://github.com/bluescarni/obake/pull/83>`__).
- Update Catch to the latest version, 2.10.2
  (`#83 <https://github.com/bluescarni/obake/pull/83>`__).
- Various docs/build system updates/improvements
  (`#82 <https://github.com/bluescarni/obake/pull/82>`__,
  `#83 <https://github.com/bluescarni/obake/pull/83>`__).

Fix
~~~

- Fix a division by zero and silence various ubsan
  warnings originating from TBB
  (`#87 <https://github.com/bluescarni/obake/pull/87>`__).
- Fix a build issue on MSVC involving Boost
  (`#86 <https://github.com/bluescarni/obake/pull/86>`__).

0.2 (2019-10-27)
----------------

New
~~~

- Various performance improvements for polynomial
  multiplication
  (`#78 <https://github.com/bluescarni/obake/pull/78>`__,
  `#79 <https://github.com/bluescarni/obake/pull/79>`__).
- Add concepts/type traits for the detection
  of bidirectional and random access iterators
  and ranges
  (`#77 <https://github.com/bluescarni/obake/pull/77>`__).

Fix
~~~

- Fix a missing include file
  (`#81 <https://github.com/bluescarni/obake/pull/81>`__).
- Fix for the compilation of the obake library with MinGW
  (`#80 <https://github.com/bluescarni/obake/pull/80>`__).
- Remove an unused variable in a lambda capture
  (`#75 <https://github.com/bluescarni/obake/pull/75>`__).

0.1 (2019-10-18)
----------------

New
~~~

- Initial release of obake.
