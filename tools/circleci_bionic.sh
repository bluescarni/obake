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
sh ../tools/circleci_install_mppp.sh

# Configure and build piranha.
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=~/.local -DPIRANHA_BUILD_TESTS=YES -DCMAKE_CXX_FLAGS="-fsanitize=undefined"
make -j2 VERBOSE=1

# Run the tests.
ctest -V

set +e
set +x
