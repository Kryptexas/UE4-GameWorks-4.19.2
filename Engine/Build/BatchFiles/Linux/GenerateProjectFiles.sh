#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "$BASH_SOURCE")" ; pwd)
# these need to be passed to children scripts
export ARCHIVE_ROOT=$HOME/Downloads
export GITHUB_TAG=4.5.0-preview

IS_GITHUB_BUILD=true
FORCE_UPDATEDEPS=false

CheckArgs()
{
  # if p4 is installed, assume building out of perforce repo (no need to download and fix dependencies)
  if which p4 > /dev/null; then
    IS_GITHUB_BUILD=false	# perforce
  fi

  # Override this with -git
  for Arg in $@; do
    if [ "$Arg" == "-git" ]; then
      IS_GITHUB_BUILD=true
    elif [ "$Arg" == "-updatedeps" ]; then
      FORCE_UPDATEDEPS=true     
    fi
  done
}


set -e

TOP_DIR=$(cd $SCRIPT_DIR/../../.. ; pwd)
cd ${TOP_DIR}

if [ ! -d Source ]; then
  echo "GenerateProjectFiles ERROR: This script file does not appear to be \
located inside the Engine/Build/BatchFiles/Linux directory."
  exit 1
fi

CheckArgs $@

if [ "$(lsb_release --id)" = "Distributor ID:	Ubuntu" -o "$(lsb_release --id)" = "Distributor ID:	Debian" -o "$(lsb_release --id)" = "Distributor ID:	Linux Mint" ]; then
  # Install all necessary dependencies
  DEPS="mono-xbuild \
    mono-dmcs \
    libmono-microsoft-build-tasks-v4.0-4.0-cil \
    libmono-system-data-datasetextensions4.0-cil
    libmono-system-web-extensions4.0-cil
    libmono-system-management4.0-cil
    libmono-system-xml-linq4.0-cil
    libmono-corlib4.0-cil
    libqt4-dev
    dos2unix
    cmake
    "

  for DEP in $DEPS; do
    if ! dpkg -s $DEP > /dev/null 2>&1; then
      echo "Attempting installation of missing package: $DEP"
      set -x
      sudo apt-get install -y $DEP
      set +x
    fi
  done
fi

echo 
if [ "$IS_GITHUB_BUILD" = true ]; then
	echo
	echo Github build
	echo Checking / downloading the latest archives
	echo
	set +e
        chmod +x Build/BatchFiles/Linux/GetAssets.py
	Build/BatchFiles/Linux/GetAssets.py EpicGames/UnrealEngine $GITHUB_TAG 
	UpdateResult=$?
	set -e

	if [ $UpdateResult -eq 1 ]; then
	  echo
          echo Could not check/download binary dependencies!
          exit 1
        fi

	# check if it had to download anything
	if [ $UpdateResult -eq 2 -o "$FORCE_UPDATEDEPS" = true ]; then
	  echo
          echo Downloaded new binaries!
	  echo Unpacking and massaging the files
	  pushd Build/BatchFiles/Linux > /dev/null
	  ./UpdateDeps.sh 
	  popd > /dev/null
        else
          echo
          echo All assets are up to date, not unpacking zip files again.
        fi

else
	echo Perforce build
	echo Assuming availability of up to date third-party libraries
fi

echo
pushd Build/BatchFiles/Linux > /dev/null
./BuildThirdParty.sh
popd > /dev/null

echo
echo Setting up Unreal Engine 4 project files...
echo

# args: wrong filename, correct filename
# expects to be in Engine folder
CreateLinkIfNoneExists()
{
    WrongName=$1
    CorrectName=$2

    pushd `dirname $CorrectName` > /dev/null
    if [ ! -f `basename $CorrectName` ] && [ -f $WrongName ]; then
      echo "$WrongName -> $CorrectName"
      ln -sf $WrongName `basename $CorrectName`
    fi
    popd > /dev/null
}



# Fixes for case sensitive filesystem.
for BASE in Content/Editor/Slate Content/Slate Documentation/Source/Shared/Icons; do
  find $BASE -name "*.PNG" | while read PNG_UPPER; do
    png_lower="$(echo "$PNG_UPPER" | sed 's/.PNG$/.png/')"
    if [ ! -f $png_lower ]; then
      PNG_UPPER=$(basename $PNG_UPPER)
      echo "$png_lower -> $PNG_UPPER"
      # link, and not move, to make it usable with Perforce workspaces
      ln -sf `basename "$PNG_UPPER"` "$png_lower"
    fi
  done
done

CreateLinkIfNoneExists ../../engine/shaders/Fxaa3_11.usf  ../Engine/Shaders/Fxaa3_11.usf
CreateLinkIfNoneExists ../../Engine/shaders/Fxaa3_11.usf  ../Engine/Shaders/Fxaa3_11.usf

set -x
xbuild Source/Programs/UnrealBuildTool/UnrealBuildTool_Mono.csproj \
  /verbosity:quiet /nologo \
  /p:TargetFrameworkVersion=v4.0 \
  /p:Configuration="Development"

xbuild Source/Programs/AutomationTool/AutomationTool_Mono.csproj \
  /verbosity:quiet /nologo \
  /p:TargetFrameworkVersion=v4.0 \
  /p:Configuration="Development"

xbuild Source/Programs/AutomationTool/Scripts/AutomationScripts.Automation.csproj \
  /verbosity:quiet /nologo \
  /p:TargetFrameworkVersion=v4.0 \
  /p:Configuration="Development"

xbuild Source/Programs/AutomationTool/Linux/Linux.Automation.csproj \
  /verbosity:quiet /nologo \
  /p:TargetFrameworkVersion=v4.0 \
  /p:Configuration="Development"

xbuild Source/Programs/AutomationTool/Android/Android.Automation.csproj \
  /verbosity:quiet /nologo \
  /p:TargetFrameworkVersion=v4.0 \
  /p:Configuration="Development"

xbuild Source/Programs/AutomationTool/HTML5/HTML5.Automation.csproj \
  /verbosity:quiet /nologo \
  /p:TargetFrameworkVersion=v4.0 \
  /p:Configuration="Development"

# pass all parameters to UBT
mono Binaries/DotNET/UnrealBuildTool.exe -makefile "$@"
set +x
