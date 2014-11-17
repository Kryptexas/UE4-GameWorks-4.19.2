#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "$BASH_SOURCE")" ; pwd)
TOP_DIR=$(cd $SCRIPT_DIR/../../.. ; pwd)
cd ${TOP_DIR}
set -e
pwd

echo "building freetype"
(
 cd Source/ThirdParty/FreeType2/FreeType2-2.4.12/src
 pwd
 make -j 5 -f ../Builds/Linux/makefile $*
 mkdir ../Lib/Linux
 cp libfreetype* ../Lib/Linux
)

echo "building recast"
(
 cd Source/ThirdParty/Recast
 pwd
 make -j 5 -f Epic/build/Linux/makefile $*
)
