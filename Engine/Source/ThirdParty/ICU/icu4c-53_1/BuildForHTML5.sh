#!/bin/bash

mkdir linuxforhtml5_build
mkdir html5_build
mkdir HTML5

cd linuxforhtml5_build
./../source/runConfigureICU Linux
make VERBOSE=1

cd ../html5_build
emconfigure ./../source/configure --disable-debug --enable-release --enable-static --disable-shared --disable-extras --disable-samples --disable-tools --disable-tests --with-data-packaging=files --build=i686-pc-linux-gnu --with-cross-build=$PWD/../linuxforhtml5_build
emmake make VERBOSE=1

function build_lib {
    local libbasename="${1##*/}"
    local finallibname="${libbasename%.a}.bc"
    echo Building $1 to $finallibname
    mkdir tmp
    cd tmp
    llvm-ar x ../$f
    for f in *.ao; do
        mv "$f" "${f%.ao}.bc";
    done
    local BC_FILES=
    for f in *.bc; do
        BC_FILES="$BC_FILES $f"
    done
    emcc $BC_FILES -o "../$finallibname"
    cd ..
    rm -rf tmp/*
    rmdir tmp
}

cd ../HTML5
for f in ../html5_build/lib/*.a; do
    build_lib $f
done
for f in ../html5_build/stubdata/*.a; do
    build_lib $f
done

cd ..
rm -rf html5_build/*
rm -rf linuxforhtml5_build/*
rmdir html5_build
rmdir linuxforhtml5_build
