#!/bin/sh

cd "`dirname "$0"`"

if [ -f ./Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh ]; then
	pushd ./Engine/Build/BatchFiles/Mac
	sh ./GenerateLLDBInit.sh
	sh ./GenerateProjectFiles.sh $@
	popd
else
	echo GenerateProjectFiles ERROR: This script does not appear to be located in the root UE4 directory and must be run from there.
fi

