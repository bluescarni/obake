#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

# Core deps.
sudo apt-get install build-essential cmake libgmp-dev libmpfr-dev wget

# Create the build dir and cd into it.
mkdir build
cd build

# Download and install mppp.
MPPP_VERSION="0.13"
wget https://github.com/bluescarni/mppp/archive/v${MPPP_VERSION}.tar.gz -O mppp.tar.gz
tar xzf mppp.tar.gz
cd mppp-${MPPP_VERSION}
mkdir build
cd build
cmake ../ -DMPPP_WITH_MPFR=YES -DMPPP_WITH_QUADMATH=YES -DCMAKE_INSTALL_PREFIX=~/.local
make install
cd ..
cd ..
rm -fr mppp-${MPPP_VERSION}

# Configure and build piranha.
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=~/.local -DPIRANHA_BUILD_TESTS=YES -DCMAKE_CXX_FLAGS="-fsanitize=undefined"
make -j2

# Run the tests.
ctest -V

set +e
set +x
