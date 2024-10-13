#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

wget https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh -O miniconda.sh
export deps_dir=$HOME/local
export PATH="$HOME/miniconda/bin:$PATH"
bash miniconda.sh -b -p $HOME/miniconda
conda create -y -p $deps_dir cmake c-compiler cxx-compiler fmt backtrace mppp \
    libboost-devel tbb-devel ninja
source activate $deps_dir

# Create the build dir and cd into it.
mkdir build
cd build

# Clear the compilation flags set up by conda.
unset CXXFLAGS
unset CFLAGS

cmake ../ -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH=$deps_dir \
    -DOBAKE_BUILD_TESTS=YES \
    -DCMAKE_CXX_FLAGS="-fsanitize=address" \
    -DOBAKE_WITH_LIBBACKTRACE=YES \
    -DCMAKE_CXX_FLAGS_DEBUG="-g -Og"

ninja -v -j4

ctest -VV -j4

set +e
set +x
