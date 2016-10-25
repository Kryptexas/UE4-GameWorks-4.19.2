#!/bin/sh
# Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
HARFBUZZ_HTML5=$(pwd)

cd ../../../../HTML5/
	. ./Build_All_HTML5_libs.rc
cd "$HARFBUZZ_HTML5"

# ----------------------------------------
# using save files so i can run this script over and over again

if [ ! -e ../CMakeLists.txt.save ]; then
	mv ../CMakeLists.txt ../CMakeLists.txt.save

	mv ../CMakeLists.txt ../CMakeLists.txt.save
	echo "SET(CMAKE_RELEASE_POSTFIX $LIB_SUFFIX)" >> ../CMakeLists.txt
fi


# ----------------------------------------
# MAKE

build_all()
{
	echo
	echo BUILDING $OPTIMIZATION
	echo

	if [ -d $MAKE_PATH$OPTIMIZATION ]; then
		rm -rf $MAKE_PATH$OPTIMIZATION
	fi
	mkdir -p $MAKE_PATH$OPTIMIZATION

	# modify (custom) CMakeLists.txt
	# output library with optimization level appended
	sed -e "s/\(add_library(harfbuzz\)/\1$LIB_SUFFIX/" ../CMakeLists.txt.save > ../CMakeLists.txt


	# modify CMAKE_TOOLCHAIN_FILE
	sed -e "s/\(EPIC_BUILD_FLAGS\} \).*-O2\"/\1$OPTIMIZATION\"/" "$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake" > $MAKE_PATH$OPTIMIZATION/Emscripten.cmake

	#note: this has been merged into Emscripten.cmake (above)
	#EMFLAGS="-msse -msse2 -s FULL_ES2=1 -s USE_PTHREADS=1"

	cd $MAKE_PATH$OPTIMIZATION
		# ./configure
		echo "Generating HarfBuzz makefile..."
		cmake -DCMAKE_TOOLCHAIN_FILE="Emscripten.cmake" -DCMAKE_BUILD_TYPE="Release" -G "Unix Makefiles" ../../BuildForUE

		# make
		echo "Building HarfBuzz..."
		emmake make clean VERBOSE=1
		emmake make VERBOSE=1 | tee log_build.txt

		# make install
#		cp -vp libXXX ../.
	cd -
}

MAKE_PATH=../../HTML5/Build

OPTIMIZATION=-O3; LIB_SUFFIX=_O3; build_all

OPTIMIZATION=-O2; LIB_SUFFIX=_O2; build_all

OPTIMIZATION=-Oz; LIB_SUFFIX=_Oz; build_all

OPTIMIZATION=-O0; LIB_SUFFIX=
build_all


# ----------------------------------------
# restore

if [ -e ../CMakeLists.txt.save ]; then
	mv ../CMakeLists.txt.save ../CMakeLists.txt
fi

