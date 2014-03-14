#!/bin/sh

echo
echo Setting up Unreal Engine 4 project files...
echo

# this is located inside an extra 'Mac' path unlike the Windows variant.

if [ ! -d ../../../Source ]; then
 echo GenerateRocketProjectFiles ERROR: This script file does not appear to be located inside the Engine/Build/BatchFiles/Mac directory.
 exit
fi

# Fix Mono if needed
CUR_DIR=`pwd`
cd "`dirname "$0"`"
sh FixMonoFiles.sh
cd "$CUR_DIR"

# setup bundled mono
CUR_DIR=`pwd`
export UE_MONO_DIR=$CUR_DIR/../../../../Engine/Binaries/ThirdParty/Mono/Mac
export PATH=$UE_MONO_DIR/bin:$PATH
export MONO_PATH=$UE_MONO_DIR/lib:$MONO_PATH

# pass all parameters to UBT
echo 
mono ../../../../Engine/Binaries/DotNET/UnrealBuildTool.exe -XcodeProjectFile -rocket "$@"
