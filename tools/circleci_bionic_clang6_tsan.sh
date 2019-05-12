#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

# Core deps.
sudo apt-get install build-essential cmake libgmp-dev libmpfr-dev wget clang libboost-dev

# Create the build dir and cd into it.
mkdir build
cd build

export CC=clang
export CXX=clang++

# Download and install mppp and abseil.
# NOTE: disable quadmath in the clang builds, because
# the inclusion of the quadmath paths from gcc breaks
# the implementation of clang's SIMD intrinsics.
export MPPP_WITH_QUADMATH=NO
bash ../tools/circleci_install_mppp.sh
bash ../tools/circleci_install_abseil.sh

# clang build.
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=~/.local -DPIRANHA_BUILD_TESTS=YES -DCMAKE_CXX_FLAGS="-fsanitize=thread"
make -j2 VERBOSE=1
# Run the tests.
ctest -V

set +e
set +x
