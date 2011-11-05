#!/bin/bash

set -e

SERD_VERSION=0.5.0
SORD_VERSION=0.5.0
LILV_VERSION=0.5.0

export PKG_CONFIG_PATH=`pwd`/build/lib/pkgconfig:$PKG_CONFIG_PATH

rm -rf build *.h *.hpp *.a
mkdir build

# Compile serd
cd serd-$SERD_VERSION
# patch -p1 -N < ../serd-dont_run_ldconfig.patch
./waf configure --prefix=../build --no-utils --static
./waf build
./waf install
cp build/*.a ..
cd ..

# Compile sord
cd sord-$SORD_VERSION
# patch -p1 -N < ../sord-dont_run_ldconfig.patch
./waf configure --prefix=../build --static
./waf build
./waf install
cp build/*.a ..
cd ..

# Compile lilv
cd lilv-$LILV_VERSION
./waf configure --prefix=../build --static
./waf build
cp build/*.a lilv/lilv.h lilv/lilvmm.hpp ..
cd ..
