#!/bin/sh

# This script gets called every time Xcode does a build or clean operation, even though it's called "Build.sh".
# Values for $ACTION: "" = building, "clean" = cleaning

# Fix Mono if needed
CUR_DIR=`pwd`
cd "`dirname "$0"`"
sh FixMonoFiles.sh
cd "$CUR_DIR"

# setup bundled mono
CUR_DIR=`pwd`
export UE_MONO_DIR=$CUR_DIR/Engine/Binaries/ThirdParty/Mono/Mac
export PATH=$UE_MONO_DIR/bin:$PATH
export MONO_PATH=$UE_MONO_DIR/lib:$MONO_PATH

case $ACTION in
	"")
		echo "Building $1..."
		xbuild /property:Configuration=Development /verbosity:quiet /nologo Engine/Source/Programs/UnrealBuildTool/UnrealBuildTool_Mono.csproj |grep -i error

                Platform=""
                AdditionalFlags="-deploy"
		                
                if [ $2 = "iphoneos" ]
                then
	                Platform="IOS"
                else
	                if [ $2 = "iphonesimulator" ]
	                then
		                Platform="IOS"
		                AdditionalFlags+=" -simulator"
	                else
		                Platform="Mac"
	                fi
                fi

		mono Engine/Binaries/DotNET/UnrealBuildTool.exe $1 $Platform $3 $AdditionalFlags "$4"
		;;
	"clean")
		echo "Cleaning $2 $3 $4..."
		xbuild /property:Configuration=Development /verbosity:quiet /nologo Engine/Source/Programs/UnrealBuildTool/UnrealBuildTool_Mono.csproj |grep -i error

                Platform=""
                AdditionalFlags="-clean"
		                
                if [ $3 = "iphoneos" ]
                then
	                Platform="IOS"
                else
	                if [ $3 = "iphonesimulator" ]
	                then
		                Platform="IOS"
		                AdditionalFlags+=" -simulator"
	                else
		                Platform="Mac"
	                fi
                fi

		mono Engine/Binaries/DotNET/UnrealBuildTool.exe $2 $Platform $4 $AdditionalFlags "$5"
		;;
esac

exit $?

