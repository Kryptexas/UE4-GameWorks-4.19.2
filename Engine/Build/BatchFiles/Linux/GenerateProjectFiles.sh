#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "$BASH_SOURCE")" ; pwd)

set -e

echo
echo Setting up Unreal Engine 4 project files...
echo

TOP_DIR=$(cd $SCRIPT_DIR/../../.. ; pwd)
cd ${TOP_DIR}

if [ ! -d Source ]; then
  echo "GenerateProjectFiles ERROR: This script file does not appear to be \
located inside the Engine/Build/BatchFiles/Linux directory."
  exit 1
fi

if [ "$(lsb_release --id)" = "Distributor ID:	Ubuntu" -o "$(lsb_release --id)" = "Distributor ID:	Debian" ]; then
  # Install all necessary dependencies
  DEPS="mono-xbuild \
    mono-dmcs \
    libmono-microsoft-build-tasks-v4.0-4.0-cil \
    libmono-system-data-datasetextensions4.0-cil
    libmono-system-web-extensions4.0-cil
    libmono-system-management4.0-cil
    libmono-system-xml-linq4.0-cil
    libmono-corlib4.0-cil
    libogg-dev"

  for DEP in $DEPS; do
    if ! dpkg -s $DEP > /dev/null 2>&1; then
      echo "Attempting installation of missing package: $DEP"
      set -x
      sudo apt-get install $DEP
      set +x
    fi
  done
fi

# args: wrong filename, correct filename
# expects to be in Engine folder
CreateLinkIfNoneExists()
{
    WrongName=$1
    CorrectName=$2

    pushd `dirname $CorrectName`
    if [ ! -f `basename $CorrectName` ]; then
      echo "$WrongName -> $CorrectName"
      ln -sf $WrongName `basename $CorrectName`
    fi
    popd
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
