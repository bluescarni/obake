.. _installation:

Installation
============

.. _requirements:

Requirements
------------

Currently, obake has the following mandatory dependencies:

* the `mp++ <https://bluescarni.github.io/mppp/>`_ multiprecision library (at least version 0.17),
* the `Boost <https://www.boost.org/>`_ C++ libraries (at least version 1.65),
* the `Abseil <https://abseil.io/>`_ C++ libraries,
* the `Intel TBB <https://github.com/intel/tbb>`__ library.

obake also depends on other libraries for optional features:

* on some operating systems, obake can use `libbacktrace <https://github.com/ianlancetaylor/libbacktrace>`_
  to improve the quality of the stack traces.

.. _installation_from_source:

Installation from source
------------------------

obake is written in `C++17 <https://en.wikipedia.org/wiki/C%2B%2B17>`_,
and thus it requires a fairly recent compiler with
robust support for modern C++ idioms. The following compilers are regularly
tested in obake's continuous integration setup:

* GCC 7 and 8 on Linux (Ubuntu bionic and cosmic),
* Clang 6 and 7 on Linux (Ubuntu bionic and cosmic),
* Visual Studio 2019 on Windows,
* Clang + Visual Studio 2015 on Windows
  (via the ``clang-cl`` driver),
* Clang 9 on OSX.

See also the
:ref:`compiler and platform specific notes <platform_specific_notes>`.

In order to install obake from source, `CMake <https://cmake.org/>`_ is
required (at least version 3.8). After downloading and unpacking obake's
source code, go to obake's
source tree, create a ``build`` directory and ``cd`` into it. E.g.,
on a Unix-like system:

.. code-block:: console

   $ cd /path/to/obake
   $ mkdir build
   $ cd build

Next, we will invoke ``cmake`` to configure the build. The following options
are currently recognised by obake's build system:

* ``OBAKE_WITH_LIBBACKTRACE``: use the `libbacktrace <https://github.com/ianlancetaylor/libbacktrace>`_
  library to improve the quality of the stack traces in Unix-like
  environments. On some Linux
  distributions, such as Ubuntu, libbacktrace is built into the GCC
  compiler; otherwise, it can be
  installed as a standalone library. Note that libbacktrace currently does not
  work on Windows (unless MinGW is being used as a compiler) and OSX.
  Defaults to ``OFF``.
* ``OBAKE_BUILD_TESTS``: build the test suite. Defaults to ``OFF``.

Additionally, there are various useful CMake variables you can set, such as:

* ``CMAKE_BUILD_TYPE``: the build type (``Release``, ``Debug``, etc.),
  defaults to ``Release``.
* ``CMAKE_INSTALL_PREFIX``: the path into which obake will be installed
  (e.g., this defaults to ``/usr/local`` on Unix-like platforms).
* ``CMAKE_PREFIX_PATH``: additional paths that will be searched by CMake
  when looking for dependencies.

Please consult `CMake's documentation <https://cmake.org/cmake/help/latest/>`_
for more details about CMake's variables and options.

A typical CMake invocation for obake may look something like this:

.. code-block:: console

   $ cmake ../ -DOBAKE_BUILD_TESTS=ON -DCMAKE_INSTALL_PREFIX=~/.local

That is, we build the test suite and we
will be installing obake into our home directory into the ``.local``
subdirectory. If CMake runs without errors, we can then proceed to actually
building obake:

.. code-block:: console

   $ cmake --build .

This command will build the obake library and, if requested, the test suite.
Next, we can install obake with the command:

.. code-block:: console

   $ cmake  --build . --target install

This command will install the obake library and header files to
the directory tree indicated by the ``CMAKE_INSTALL_PREFIX`` variable.

If enabled, the test suite can be executed with the command:

.. code-block:: console

   $ cmake  --build . --target test

.. note::

   On Windows, in order to execute the test suite you have to ensure that the
   ``PATH`` variable includes the directory that contains the obake
   DLL (otherwise the tests will fail to run).

Troubleshooting
^^^^^^^^^^^^^^^

By far, the most common problem when compiling obake is the detection
of the dependencies.

On Linux systems, generally speaking, the best way of installing the
dependencies is through the distribution's package manager
(e.g., ``apt-get`` on Ubuntu).
For those dependencies not available from the system's package
manager (e.g., mp++, libbacktrace, etc.), the best course of action
is to install them by hand in the user's home directory under the
``.local`` subdirectory, and then set the CMake variable
``CMAKE_PREFIX_PATH`` to ``~/.local``. This should be enough for
obake's build system to successfully locate the dependencies in most
cases.

On Windows and OSX, the dependencies are best handled with a 3rd party
package manager, such as `Conda <https://docs.conda.io/en/latest/>`_
(for both OSX and Windows) or `Homebrew <https://brew.sh/>`_ (only
for OSX). When using 3rd party package managers, it might be necessary
to set the ``CMAKE_PREFIX_PATH`` variable to the root path of the
package manager's install tree in order
for obake's build system to correctly locate the dependencies.

.. _platform_specific_notes:

Compiler and platform specific notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Visual Studio:

* Due to various compiler issues, only MSVC 2019 is currently able
  to compile obake.
* It is possible to use ``clang-cl`` to compile obake
  with earlier versions of MSVC (2017 and 2015). This means
  that Clang will be used as a C/C++ compiler, while the
  C++ standard library will be the one supplied with MSVC. Be
  aware that the C++ library from MSVC 2015 is not
  fully C++17-compliant, and as a result
  certain features in obake will be disabled when using
  MSVC 2015 (these occurrences are detailed in the API
  documentation).
* When using Visual Studio, the obake library is compiled
  with the ``NOMINMAX`` and ``WIN32_LEAN_AND_MEAN`` definitions,
  and, if supported, with the ``/permissive-`` compiler flag.

GCC:

* Due to a compiler bug, when using GCC 7 obake's customisation points
  do not have any ``noexcept`` specifier.

Clang:

* Due to a compiler bug, Clang 8.0.0 may fail to compile obake's
  test suite with debugging information. The issue appears to have been
  rectified in Clang 8.0.1.

OSX:

* On OSX, only the most recent versions of Xcode (i.e., Xcode 9 or later)
  are capable of compiling obake.
  As an alternative to Xcode, one can install a more modern compiler toolchain
  using package managers such as `Conda <https://docs.conda.io/en/latest/>`_ or
  `Homebrew <https://brew.sh/>`_.

Building the documentation
^^^^^^^^^^^^^^^^^^^^^^^^^^

obake's documentation is built with a tool called `Sphinx <https://www.sphinx-doc.org/>`_,
and it uses a `custom theme <https://sphinx-rtd-theme.readthedocs.io/en/stable/>`_.
Sphinx can typically be installed from a variety of package managers,
while the custom theme can easily be installed with ``pip``:

.. code-block:: console

   $ pip install --user sphinx_rtd_theme

Before attempting to build the documentation, you must ensure
to run CMake from the ``build`` directory at least once
(see the :ref:`source installation instructions <installation_from_source>`):

.. code-block:: console

   $ cmake ../

Running CMake is necessary to generate the configuration files required
to build the documentation.

After having run CMake, you can move to the ``doc`` directory and proceed
to build the documentation. Executing the command

.. code-block:: console

   $ make html

will produce the documentation in HTML format. The documentation will be
generated in the ``doc/_build`` directory.

Packages
--------

Conda
^^^^^

obake is available in the `conda <https://conda.io/en/latest/>`__ package manager from the
`conda-forge <https://conda-forge.org/>`__ channel. Two
packages are available:

* `obake <https://anaconda.org/conda-forge/obake>`__, which contains the obake shared library,
* `obake-devel <https://anaconda.org/conda-forge/obake-devel>`__,
  which contains the obake headers and the
  CMake support files.

In order to install obake via conda, you just need
to add ``conda-forge`` to the channels:

.. code-block:: console

   $ conda config --add channels conda-forge
   $ conda config --set channel_priority strict
   $ conda install obake obake-devel

Please refer to the `conda documentation <https://conda.io/en/latest/>`__ for instructions on how to setup and manage
your conda installation.
