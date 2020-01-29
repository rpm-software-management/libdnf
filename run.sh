#!/bin/sh

# lukash: jeez write a makefile :)

set -e

export CC=clang
export CXX=clang

#export CC=gcc
#export CXX=c++

rm -rf build
mkdir build || :
pushd build

# configure
#BINDINGS_OFF="-DWITH_GO=OFF -DWITH_PYTHON3=OFF -DWITH_PERL5=OFF -DWITH_RUBY=OFF"
SANITIZERS="-DWITH_SANITIZERS=ON"
cmake .. $BINDINGS_OFF $SANITIZERS

# compile
make

pushd doc
#make doc
popd


#./main


# run tests
export CTEST_OUTPUT_ON_FAILURE=1
make test

popd
