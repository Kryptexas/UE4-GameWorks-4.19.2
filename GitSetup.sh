#!/bin/sh
# Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

set -e

cd "`dirname "$0"`"

if [ ! -f Engine/Binaries/DotNET/GitDependencies.exe ]; then
	echo "GitSetup ERROR: This script does not appear to be located \
       in the root UE4 directory and must be run from there."
	exit 1
fi 

# Setup the git hooks
if [ -d .git/hooks ]; then
	echo Registering git hooks...
	echo \#!/bin/sh >.git/hooks/post-checkout
	echo mono Engine/Binaries/DotNET/GitDependencies.exe >>.git/hooks/post-checkout
	chmod +x .git/hooks/post-checkout
fi

#if [ "$(uname)" = "Darwin" ]; then
#	cd Engine/Build/BatchFiles/Mac
#	source SetupMono.sh "`pwd`"
#	cd ../../../..
#fi

mono Engine/Binaries/DotNET/GitDependencies.exe "$@"
