#!/bin/bash

set -e

NUM_CPU=`grep -c ^processor /proc/cpuinfo`
UE_THIRD_PARTY=`cd $(pwd)/../../../../../../Source/ThirdParty; pwd`
CXX_FLAGS="-fvisibility=hidden -std=c++11 -fPIC -I$UE_THIRD_PARTY/Linux/LibCxx/include -I$UE_THIRD_PARTY/Linux/LibCxx/include/c++/v1 -I$(pwd)/AlembicDeploy/include"
CXX_LINKER="-nodefaultlibs -L$UE_THIRD_PARTY/Linux/LibCxx/lib/Linux/x86_64-unknown-linux-gnu/ -L$UE_THIRD_PARTY/openexr/Deploy/lib/Linux/StaticRelease/x86_64-unknown-linux-gnu/ $UE_THIRD_PARTY/Linux/LibCxx/lib/Linux/x86_64-unknown-linux-gnu/libc++.a $UE_THIRD_PARTY/Linux/LibCxx/lib/Linux/x86_64-unknown-linux-gnu/libc++abi.a -lm -lc -lgcc_s -lgcc"

export ILMBASE_ROOT="$(pwd)/AlembicDeploy/Linux/lib/x86_64-unknown-linux-gnu"
export HDF5_ROOT="$(pwd)/temp/Linux"

mkdir -p build/Linux
cd build/Linux
rm -rf alembic
mkdir alembic
cd alembic
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="$CXX_FLAGS" -DALEMBIC_ILMBASE_INCLUDE_DIRECTORY="$(pwd)../../../AlembicDeploy/include" -DCMAKE_EXE_LINKER_FLAGS="$CXX_LINKER" -DCMAKE_MODULE_LINKER_FLAGS="$CXX_LINKER" -DCMAKE_SHARED_LINKER_FLAGS="$CXX_LINKER" -DZLIB_INCLUDE_DIR=$UE_THIRD_PARTY/zlib/v1.2.8/include/Linux/x86_64-unknown-linux-gnu/ -DZLIB_LIBRARY=$UE_THIRD_PARTY/zlib/v1.2.8/lib/Linux/x86_64-unknown-linux-gnu/ -DALEMBIC_SHARED_LIBS=OFF -DUSE_TESTS=OFF -DUSE_BINARIES=OFF  -DUSE_HDF5=ON -DALEMBIC_ILMBASE_LINK_STATIC=ON -DCMAKE_INSTALL_PREFIX=$(pwd)/../../../temp/Linux --config Release ../../../alembic/
make -j$NUM_CPU install
cd ../../../
