#!/bin/sh

## Unreal Engine 4 Mono setup script
## Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

## This script is expecting to exist in the UE4/Engine/Build/BatchFiles directory.  It will not work correctly
## if you copy it to a different location and run it.

echo
echo Running Mono...
echo

# Fix Mono if needed
CUR_DIR=`pwd`
cd "`dirname "$0"`"
sh FixMonoFiles.sh
cd "$CUR_DIR"

# put ourselves into Engine directory (two up from location of this script)
pushd "`dirname "$0"`/../../.."

# setup bundled mono
CUR_DIR=`pwd`
export UE_MONO_DIR=$CUR_DIR/Binaries/ThirdParty/Mono/Mac
export PATH=$UE_MONO_DIR/bin:$PATH
export MONO_PATH=$UE_MONO_DIR/lib:$MONO_PATH

if [ ! -f Build/BatchFiles/Mac/RunMono.sh ]; then
	echo RunMono ERROR: The batch file does not appear to be located in the /Engine/Build/BatchFiles directory.  This script must be run from within that directory.
	exit 1
fi

mono "$@"
exit $?
