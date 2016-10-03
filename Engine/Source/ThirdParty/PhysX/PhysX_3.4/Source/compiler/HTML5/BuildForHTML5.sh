#!/bin/sh
# Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

# WARNING: connot build libs if absolute paths contains any space -- will revisit this in the future...

#set -x


# ----------------------------------------
# env

cd ../../../..
export GW_DEPS_ROOT=$(pwd)

cd ../Engine/Source/ThirdParty/HTML5/
	# source in emscripten SDK paths and env vars
	. ./Build_All_HTML5_libs.rc
cd "$GW_DEPS_ROOT"


# ----------------------------------------
# MAKE

build_all()
{
	echo
	echo BUILDING $OPTIMIZATION
	echo

	if [ ! -d $MAKE_PATH$OPTIMIZATION ]; then
		mkdir -p $MAKE_PATH$OPTIMIZATION
	fi

	# modify emscripten CMAKE_TOOLCHAIN_FILE
	sed -e "s/\(FLAGS_RELEASE \)\".*-O2\"/\1\"$OPTIMIZATION\"/" "$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake" > $MAKE_PATH$OPTIMIZATION/Emscripten.cmake


	cd $MAKE_PATH$OPTIMIZATION
		echo "Generating PhysX makefile..."
		export CMAKE_MODULE_PATH="$GW_DEPS_ROOT/Externals/CMakeModules"
		cmake -DCMAKE_TOOLCHAIN_FILE="Emscripten.cmake" -DCMAKE_BUILD_TYPE="Release" -DTARGET_BUILD_PLATFORM="HTML5" \
			-DPXSHARED_ROOT_DIR="$GW_DEPS_ROOT/PxShared" \
			-DNVSIMD_INCLUDE_DIR="$GW_DEPS_ROOT/PxShared/src/NvSimd" \
			-DNVTOOLSEXT_INCLUDE_DIRS="$GW_DEPS_ROOT/PhysX_3.4/externals/nvToolsExt/include" \
			-G "Unix Makefiles" ../../..

		echo "Building PhysX ..."
		emmake make clean VERBOSE=1
		emmake make VERBOSE=1 | tee log_build.txt
	cd -
}

cd PhysX_3.4

	MAKE_PATH=lib/HTML5/Build
	
	OPTIMIZATION=-O3; export LIB_SUFFIX=_O3; build_all
	
	OPTIMIZATION=-O2; export LIB_SUFFIX=_O2; build_all
	
	OPTIMIZATION=-Oz; export LIB_SUFFIX=_Oz; build_all
	
	OPTIMIZATION=-O0; export LIB_SUFFIX=""
	build_all


	# ----------------------------------------
	# install

	if [ ! -d Bin/HTML5 ]; then
		mkdir -p Bin/HTML5
	fi
	cp -vp lib/cmake/* Bin/HTML5/.
	pwd

cd ..

