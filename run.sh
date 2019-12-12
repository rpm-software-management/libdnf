#!/bin/sh

# lukash: jeez write a makefile :)

set -e

export CC=clang
export CXX=clang

rm -rf build
mkdir build
pushd build

# configure
cmake ..

# compile
make

pushd doc
make doc
popd


./main


# run tests
export CTEST_OUTPUT_ON_FAILURE=1
make test

popd
