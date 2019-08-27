#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

wget https://repo.anaconda.com/miniconda/Miniconda3-latest-MacOSX-x86_64.sh -O miniconda.sh;
export deps_dir=$HOME/local
export PATH="$HOME/miniconda/bin:$PATH"
bash miniconda.sh -b -p $HOME/miniconda
conda config --add channels conda-forge --force

conda_pkgs="cmake>=3.3 mppp boost-cpp"

conda create -q -p $deps_dir -y $conda_pkgs
source activate $deps_dir

export deps_dir=$HOME/local
export PATH="$HOME/miniconda/bin:$PATH"
export PATH="$deps_dir/bin:$PATH"

export CXX=clang++
export CC=clang

git clone https://github.com/abseil/abseil-cpp.git
cd abseil-cpp
git checkout 3c98fcc0461bd2a4b9c149d4748a7373a225cf4b
mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_CXX_STANDARD=17
make install -j2 VERBOSE=1
cd ..
cd ..
rm -fr abseil-cpp

cmake -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=Debug -DPIRANHA_BUILD_TESTS=yes ../
make -j2 VERBOSE=1
ctest -V

set +e
set +x
