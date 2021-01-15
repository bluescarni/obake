#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

# Core deps.
sudo apt-get install build-essential

wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O miniconda.sh;
export deps_dir=$HOME/local
bash miniconda.sh -b -p $HOME/miniconda
export PATH="$HOME/miniconda/bin:$PATH"
conda config --add channels conda-forge
conda config --set channel_priority strict
conda_pkgs="cmake mppp boost-cpp tbb tbb-devel abseil-cpp backtrace fmt"
conda create -q -p $deps_dir -y $conda_pkgs
source activate $deps_dir

# Create the build dir and cd into it.
mkdir build
cd build

# GCC build.
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=~/.local -DOBAKE_BUILD_TESTS=YES -DCMAKE_CXX_FLAGS="--coverage" -DOBAKE_WITH_LIBBACKTRACE=YES -DBoost_NO_BOOST_CMAKE=ON
make -j2 VERBOSE=1
# Run the tests.
ctest -V
# Upload coverage data.
bash <(curl -s https://codecov.io/bash) -x gcov-9 > /dev/null

set +e
set +x
