#!/bin/sh
# Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

# WARNING: connot build libs if absolute paths contains any space -- will revisit this in the future...

#set -x


# ----------------------------------------
# env

cd ../../../..
export GW_DEPS_ROOT=$(pwd)

cd ../HTML5/
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
		echo "Generating $MAKETARGET makefile..."
		export CMAKE_MODULE_PATH="$GW_DEPS_ROOT/Externals/CMakeModules"
		cmake -DCMAKE_TOOLCHAIN_FILE="Emscripten.cmake" -DCMAKE_BUILD_TYPE="Release" -DTARGET_BUILD_PLATFORM="HTML5" \
			-DPHYSX_ROOT_DIR="$GW_DEPS_ROOT/PhysX_3.4" \
			-DPXSHARED_ROOT_DIR="$GW_DEPS_ROOT/PxShared" \
			-DNVSIMD_INCLUDE_DIR="$GW_DEPS_ROOT/PxShared/src/NvSimd" \
			-DNVTOOLSEXT_INCLUDE_DIRS="$GW_DEPS_ROOT/PhysX_3.4/externals/nvToolsExt/include" \
			$CustomFlags -G "Unix Makefiles" "$GW_DEPS_ROOT/$MAKETARGET/compiler/cmake/HTML5"

		echo "Building $MAKETARGET ..."
		emmake make clean VERBOSE=1
		emmake make VERBOSE=1 | tee log_build.txt
	cd -
}


build_pxshared()
{
	MAKETARGET=PxShared/src
	MAKE_PATH=PxShared/lib/HTML5/Build
	
	OPTIMIZATION=-O3; export LIB_SUFFIX=_O3; build_all

	OPTIMIZATION=-O2; export LIB_SUFFIX=_O2; build_all
	
	OPTIMIZATION=-Oz; export LIB_SUFFIX=_Oz; build_all
	
	OPTIMIZATION=-O0; export LIB_SUFFIX=""
	build_all
}

build_phsyx()
{
	MAKETARGET=PhysX_3.4/Source
	MAKE_PATH=PhysX_3.4/lib/HTML5/Build
	
	OPTIMIZATION=-O3; export LIB_SUFFIX=_O3; build_all

	OPTIMIZATION=-O2; export LIB_SUFFIX=_O2; build_all
	
	OPTIMIZATION=-Oz; export LIB_SUFFIX=_Oz; build_all
	
	OPTIMIZATION=-O0; export LIB_SUFFIX=""
	build_all
}

build_apex()
{
	MAKETARGET=APEX_1.4
	MAKE_PATH=APEX_1.4/lib/HTML5/Build
	CustomFlags="-DAPEX_ENABLE_UE4=1"
	
	OPTIMIZATION=-O3; export LIB_SUFFIX=_O3; build_all

	OPTIMIZATION=-O2; export LIB_SUFFIX=_O2; build_all
	
	OPTIMIZATION=-Oz; export LIB_SUFFIX=_Oz; build_all
	
	OPTIMIZATION=-O0; export LIB_SUFFIX=""
	build_all
}

#build_pxshared
#build_phsyx
# seems - apex ==> includes physx ==> which includes pxshared
# meaning, just build apex - it will build the others
build_apex


# ----------------------------------------
# install

find APEX_1.4/lib/HTML5 -type f -name "*.bc" -exec cp "{}" Lib/HTML5/. \;
#find PhysX_3.4/lib/HTML5 -type f -name "*.bc" -exec cp "{}" Lib/HTML5/. \;
#find PxShared/lib/HTML5 -type f -name "*.bc" -exec cp "{}" Lib/HTML5/. \;

