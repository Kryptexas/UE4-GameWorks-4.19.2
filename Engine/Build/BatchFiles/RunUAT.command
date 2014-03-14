#!/bin/sh

## Unreal Engine 4 AutomationTool setup script
## Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

## This script is expecting to exist in the UE4/Engine/Build/BatchFiles directory.  It will not work correctly
## if you copy it to a different location and run it.

echo
echo Running AutomationTool...
echo

# Fix Mono if needed
CUR_DIR=`pwd`
cd "`dirname "$0"`/Mac"
sh FixMonoFiles.sh
cd "$CUR_DIR"

# put ourselves into Engine directory (two up from location of this script)
pushd "`dirname "$0"`/../.."

# setup bundled mono
echo Setup bundled mono
CUR_DIR=`pwd`
export UE_MONO_DIR=$CUR_DIR/Binaries/ThirdParty/Mono/Mac
export PATH=$UE_MONO_DIR/bin:$PATH
export MONO_PATH=$UE_MONO_DIR/lib:$MONO_PATH

UATDirectory=Binaries/DotNET/
UATNoCompileArg=
echo Mono Setup completed
if [ ! -f Build/BatchFiles/RunUAT.command ]; then
	echo RunUAT ERROR: The batch file does not appear to be located in the /Engine/Build/BatchFiles directory.  This script must be run from within that directory.
	exit 1
fi

# see if we have the no compile arg
echo Checking for -NoCompile
args=“$@“
HAS_NO_COMPILE=`echo $args | grep -i nocompile`
if [ "$HAS_NO_COMPILE" == "$args" ]; then
	UATNoCompileArg=-NoCompile
else
	UATNoCompileArg=
fi

# see if the .csproj exists to be compiled
# @todo: Check for an override envvar: if not "%ForcePrecompiledUAT%"=="" goto NoVisualStudio2010Environment
if [ "$UATNoCompileArg" != "-NoCompile" ]; then
	if [ ! -f Source/Programs/AutomationTool/AutomationTool_Mono.csproj ]; then
		echo No project to compile, attempting to use precompiled AutomationTool
		UATNoCompileArg=-NoCompile
	else
		echo Check if $UATDirectory contains AutomationTool.exe
		if [ -f "$UATDirectory/AutomationTool.exe" ]; then
			echo AutomationTool exists: Deleting
			rm -f $UATDirectory/AutomationTool.exe
			if [ -d "$UATDirectory/AutomationScripts" ]; then
				echo Deleting all AutomationScript dlls
				rm -rf $UATDirectory/AutomationScripts
			fi
		fi
		echo Compiling AutomationTool with xbuild
		xbuild Source/Programs/AutomationTool/AutomationTool_Mono.csproj /property:Configuration=Development /property:Platform=AnyCPU /property:TargetFrameworkVersion=v4.5 /tv:4.0 /verbosity:quiet /nologo |grep -i error
		# make sure it succeeded
		if [ $? -ne 1 ]; then
			echo RunUAT ERROR: AutomationTool failed to compile.
			exit 1
		else
			echo Compilation Succeeded
		fi
	fi
fi

## Run AutomationTool

#run UAT
pushd $UATDirectory
if [ -z "$uebp_LogFolder" ]; then
LogDir=$HOME/Library/Logs/Unreal\ Engine/LocalBuildLogs
else
	LogDir="$uebp_LogFolder"
fi
# you can't set a dotted env var nicely in sh, but env will run a command with a list of env vars set, including dotted ones
echo Start UAT: mono AutomationTool.exe "$@"
env uebp_LogFolder="$LogDir" mono AutomationTool.exe "$@" $UATNoCompileArg
UATReturn=$?

# @todo: Copy log files to somewhere useful
# if not "%uebp_LogFolder%" == "" copy log*.txt %uebp_LogFolder%\UAT_*.*
# if "%uebp_LogFolder%" == "" copy log*.txt c:\LocalBuildLogs\UAT_*.*
cp log*.txt /var/log

if [ $UATReturn -ne 0 ]; then
	echo RunUAT ERROR: AutomationTool was unable to run successfully.
	exit 1
fi
