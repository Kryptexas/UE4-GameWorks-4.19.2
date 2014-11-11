#!/bin/sh
# Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

set -e

cd "`dirname "$0"`"

if [ ! -f ../../../Binaries/DotNET/GitDependencies.exe ]; then
	echo "Cannot find GitDependencies.exe. This script should be placed in Engine/Build/BatchFiles/Mac."
	exit 1
fi 

echo from `pwd`

source SetupMono.sh "`pwd`"

cd ../../../..

mono Engine/Binaries/DotNET/GitDependencies.exe "$@"
