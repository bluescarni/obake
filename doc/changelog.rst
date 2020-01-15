Changelog
=========

0.4.0 (unreleased)
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
  (`#98 <https://github.com/bluescarni/obake/pull/98>`__).
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

0.3 (31-10-2019)
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

0.2 (27-10-2019)
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

0.1 (18-10-2019)
----------------

New
~~~

- Initial release of obake.
