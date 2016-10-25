#!/bin/sh

VORBIS_HTML5=$(pwd)

cd ../../../../HTML5/
	. ./Build_All_HTML5_libs.rc
cd "$VORBIS_HTML5"


cd ../../lib

proj1=libvorbis
proj2=libvorbisfile
makefile=../build/HTML5/Makefile.HTML5
EMFLAGS="-msse -msse2 -s FULL_ES2=1 -s USE_PTHREADS=1"

make clean   OPTIMIZATION=-O3 CFLAGS_EXTRA="$EMFLAGS" LIB1=${proj1}_O3.bc LIB2=${proj2}_O3.bc -f ${makefile}
make install OPTIMIZATION=-O3 CFLAGS_EXTRA="$EMFLAGS" LIB1=${proj1}_O3.bc LIB2=${proj2}_O3.bc -f ${makefile}

make clean   OPTIMIZATION=-O2 CFLAGS_EXTRA="$EMFLAGS" LIB1=${proj1}_O2.bc LIB2=${proj2}_O2.bc -f ${makefile}
make install OPTIMIZATION=-O2 CFLAGS_EXTRA="$EMFLAGS" LIB1=${proj1}_O2.bc LIB2=${proj2}_O2.bc -f ${makefile}

make clean   OPTIMIZATION=-Oz CFLAGS_EXTRA="$EMFLAGS" LIB1=${proj1}_Oz.bc LIB2=${proj2}_Oz.bc -f ${makefile}
make install OPTIMIZATION=-Oz CFLAGS_EXTRA="$EMFLAGS" LIB1=${proj1}_Oz.bc LIB2=${proj2}_Oz.bc -f ${makefile}

make clean   OPTIMIZATION=-O0 CFLAGS_EXTRA="$EMFLAGS" LIB1=${proj1}.bc LIB2=${proj2}.bc -f ${makefile}
make install OPTIMIZATION=-O0 CFLAGS_EXTRA="$EMFLAGS" LIB1=${proj1}.bc LIB2=${proj2}.bc -f ${makefile}

ls -l ../lib/HTML5

