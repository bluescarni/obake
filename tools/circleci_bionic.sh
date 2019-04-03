#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

# Core deps.
sudo apt-get install build-essential cmake libgmp-dev libmpfr-dev wget clang libc++-dev

# Create the build dir and cd into it.
mkdir build
cd build

# Download and install mppp.
sh ../tools/circleci_install_mppp.sh

# GCC build.
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=~/.local -DPIRANHA_BUILD_TESTS=YES -DCMAKE_CXX_FLAGS="-fsanitize=undefined"
make -j2 VERBOSE=1
# Run the tests.
ctest -V

# clang build.
cd ..
mkdir build_clang
cd build_clang
CC=clang CXX=clang++ cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=~/.local -DPIRANHA_BUILD_TESTS=YES
make -j2 VERBOSE=1
# Run the tests.
ctest -V

set +e
set +x
