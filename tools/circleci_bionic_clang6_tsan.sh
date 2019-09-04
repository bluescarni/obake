#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

# Core deps.
sudo apt-get install build-essential cmake libgmp-dev libmpfr-dev wget clang libboost-dev libtbb-dev

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
cmake ../ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=~/.local -DPIRANHA_BUILD_TESTS=YES -DCMAKE_CXX_FLAGS="-fsanitize=thread"
make -j2 VERBOSE=1
# Run the tests.
# Enable the custom suppression file to deal with
# various noise coming from TBB.
TSAN_OPTIONS="suppressions=/home/circleci/project/tools/tsan.supp history_size=7" ctest -V

set +e
set +x
