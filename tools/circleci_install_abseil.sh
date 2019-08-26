#!/usr/bin/env bash

# Echo each command
set -x

# Exit on error.
set -e

git clone https://github.com/abseil/abseil-cpp.git
cd abseil-cpp
git checkout 3c98fcc0461bd2a4b9c149d4748a7373a225cf4b
mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=~/.local -DCMAKE_CXX_STANDARD=17
make install -j2 VERBOSE=1
cd ..
cd ..
rm -fr abseil-cpp

set +e
set +x
