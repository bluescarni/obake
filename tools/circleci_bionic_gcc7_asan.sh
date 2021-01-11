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

cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=~/.local -DCMAKE_PREFIX_PATH=~/.local -DOBAKE_BUILD_TESTS=YES -DCMAKE_CXX_FLAGS="-march=native -fsanitize=address" -DOBAKE_WITH_LIBBACKTRACE=YES -DBoost_NO_BOOST_CMAKE=ON
make -j2 VERBOSE=1
make install
# Run the tests. Enable the custom suppression file for ASAN
# in order to suppress spurious warnings from TBB code.
LSAN_OPTIONS=suppressions=/home/circleci/project/tools/lsan.supp ctest -j4 -V

set +e
set +x
