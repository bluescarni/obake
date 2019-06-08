#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

git clone https://github.com/bluescarni/mppp.git
cd mppp
mkdir build
cd build
cmake ../ -DMPPP_WITH_MPFR=YES -DMPPP_WITH_QUADMATH=${MPPP_WITH_QUADMATH} -DCMAKE_INSTALL_PREFIX=~/.local -DCMAKE_CXX_STANDARD=17
make install -j2
cd ..
cd ..
rm -fr mppp

set +e
set +x
