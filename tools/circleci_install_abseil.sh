#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

git clone https://github.com/abseil/abseil-cpp.git
cd abseil-cpp
mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=~/.local
make install -j2
cd ..
cd ..
rm -fr abseil-cpp

set +e
set +x
