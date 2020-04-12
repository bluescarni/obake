#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

wget https://repo.anaconda.com/miniconda/Miniconda3-latest-MacOSX-x86_64.sh -O miniconda.sh;
export deps_dir=$HOME/local
bash miniconda.sh -b -p $HOME/miniconda
export PATH="$HOME/miniconda/bin:$PATH"
conda config --add channels conda-forge
conda config --set channel_priority strict
# NOTE: the clang pins are hopefully temporary.
conda_pkgs="cmake>=3.3 mppp boost-cpp tbb tbb-devel clang<10 clangdev<10 abseil-cpp"
conda create -q -p $deps_dir -y $conda_pkgs
source activate $deps_dir

export CXX=clang++
export CC=clang

cmake ../ -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=${OBAKE_BUILD_TYPE} -DOBAKE_BUILD_TESTS=yes
make -j2 VERBOSE=1
make install
ctest -j4 -V

set +e
set +x
