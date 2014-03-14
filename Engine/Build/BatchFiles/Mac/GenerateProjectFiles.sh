#!/bin/sh

echo
echo Setting up Unreal Engine 4 project files...
echo

# this is located inside an extra 'Mac' path unlike the Windows variant.

if [ ! -d ../../../Binaries/DotNET ]; then
 echo GenerateProjectFiles ERROR: It looks like you're missing some files that are required in order to generate projects.  Please check that you've downloaded and unpacked the engine source code, binaries, content and third-party dependencies before running this script.
 exit
fi

if [ ! -d ../../../Source ]; then
 echo GenerateProjectFiles ERROR: This script file does not appear to be located inside the Engine/Build/BatchFiles/Mac directory.
 exit
fi

# Fix Mono if needed
CUR_DIR=`pwd`
cd "`dirname "$0"`"
sh FixMonoFiles.sh
cd "$CUR_DIR"

# setup bundled mono
CUR_DIR=`pwd`
export UE_MONO_DIR=$CUR_DIR/../../../Binaries/ThirdParty/Mono/Mac
export PATH=$UE_MONO_DIR/bin:$PATH
export MONO_PATH=$UE_MONO_DIR/lib:$MONO_PATH

xbuild ../../../Source/Programs/UnrealBuildTool/UnrealBuildTool_Mono.csproj /property:Configuration="Development" /verbosity:quiet /nologo |grep -i error

# pass all parameters to UBT
mono ../../../Binaries/DotNET/UnrealBuildTool.exe -XcodeProjectFile "$@"
