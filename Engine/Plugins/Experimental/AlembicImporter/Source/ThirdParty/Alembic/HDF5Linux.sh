#!/bin/bash

set -e

NUM_CPU=`grep -c ^processor /proc/cpuinfo`
UE_THIRD_PARTY=`cd $(pwd)/../../../../../../Source/ThirdParty; pwd`
C_FLAGS="-fvisibility=hidden -fPIC"

mkdir -p build/Linux
cd build/Linux
rm -rf hdf5
mkdir hdf5
cd hdf5
cmake -G "Unix Makefiles" -DCMAKE_C_FLAGS="$C_FLAGS" -DZLIB_INCLUDE_DIR=$UE_THIRD_PARTY/zlib/v1.2.8/include/Linux/x86_64-unknown-linux-gnu/ -DZLIB_LIBRARY=$UE_THIRD_PARTY/zlib/v1.2.8/lib/Linux/x86_64-unknown-linux-gnu/ -DBUILD_SHARED_LIBS=OFF -DHDF5_BUILD_CPP_LIB=OFF -DHDF5_BUILD_HL_LIB=OFF -DHDF5_BUILD_EXAMPLES=OFF -DHDF5_BUILD_TOOLS=OFF -DHDF5_EXTERNAL_LIB_PREFIX="" -DHDF5_ENABLE_Z_LIB_SUPPORT=ON -DHDF5_ENABLE_THREADSAFE=ON -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=$(pwd)/../../../temp/Linux ../../../hdf5
make -j$NUM_CPU install
cd ../../../
