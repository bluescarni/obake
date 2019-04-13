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

# Download and install mppp.
bash ../tools/circleci_install_mppp.sh

# clang build.
CC=clang CXX=clang++ cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=~/.local -DPIRANHA_BUILD_TESTS=YES -DCMAKE_CXX_FLAGS="-fsanitize=thread" -DQuadmath_INCLUDE_DIR=/usr/lib/gcc/x86_64-linux-gnu/7/include/ -DQuadmath_LIBRARY=/usr/lib/gcc/x86_64-linux-gnu/7/libquadmath.so
make -j2 VERBOSE=1
# Run the tests.
ctest -V

set +e
set +x
