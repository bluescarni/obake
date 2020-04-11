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
conda_pkgs="cmake>=3.3 mppp boost-cpp tbb tbb-devel abseil-cpp backtrace sphinx sphinx_rtd_theme"
conda create -q -p $deps_dir -y $conda_pkgs
source activate $deps_dir

# Create the build dir and cd into it.
mkdir build
cd build

cmake ../ -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=Release -DOBAKE_BUILD_TESTS=yes -DOBAKE_WITH_LIBBACKTRACE=yes -DOBAKE_BUILD_BENCHMARKS=yes
make -j2 VERBOSE=1
ctest -j4 -V

# Build the documentation.
cd ../doc
export SPHINX_OUTPUT=`make html linkcheck 2>&1 | grep -v "is deprecated" >/dev/null`;

if [[ "${SPHINX_OUTPUT}" != "" ]]; then
    echo "Sphinx encountered some problem:";
    echo "${SPHINX_OUTPUT}";
    exit 1;
fi
echo "Sphinx ran successfully";

if [[ ! -z "${CI_PULL_REQUEST}" ]]; then
    echo "Testing a pull request, the generated documentation will not be uploaded.";
    exit 0;
fi

if [[ "${CIRCLE_BRANCH}" != "master" ]]; then
    echo "Branch is not master, the generated documentation will not be uploaded.";
    exit 0;
fi

# Check out the gh_pages branch in a separate dir.
cd ../..
git config --global push.default simple
git config --global user.name "CircleCI"
git config --global user.email "bluescarni@gmail.com"
set +x
git clone "https://${GH_ACCESS_TOKEN}@github.com/bluescarni/obake.git" obake_gh_pages -q
set -x
cd obake_gh_pages
git checkout -b gh-pages --track origin/gh-pages;
git rm -fr *;
mv ../project/doc/_build/html/* .;
git add *;
# We assume here that a failure in commit means that there's nothing
# to commit.
git commit -m "Update Sphinx documentation, commit ${CIRCLE_SHA1} [skip ci]." || exit 0
PUSH_COUNTER=0
until git push -q
do
    git pull -q
    PUSH_COUNTER=$((PUSH_COUNTER + 1))
    if [ "$PUSH_COUNTER" -gt 3 ]; then
        echo "Push failed, aborting.";
        exit 1;
    fi
done

set +e
set +x
