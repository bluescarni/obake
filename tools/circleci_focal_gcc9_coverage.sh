#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

# Core deps.
sudo apt-get install build-essential cmake libgmp-dev libmpfr-dev wget curl libboost-dev libboost-serialization-dev libtbb-dev

# Create the build dir and cd into it.
mkdir build
cd build

# Download and install mppp and abseil
export MPPP_WITH_QUADMATH=YES
bash ../tools/circleci_install_mppp.sh
bash ../tools/circleci_install_abseil.sh

# GCC build.
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=~/.local -DOBAKE_BUILD_TESTS=YES -DCMAKE_CXX_FLAGS="-fconcepts --coverage" -DOBAKE_WITH_LIBBACKTRACE=YES
make -j2 VERBOSE=1
# Run the tests.
ctest -V
# Upload coverage data.
bash <(curl -s https://codecov.io/bash) -x gcov-9 > /dev/null

set +e
set +x
