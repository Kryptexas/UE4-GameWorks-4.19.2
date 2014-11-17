#!/bin/sh

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


if [ "$(lsb_release --id)" = "Distributor ID:	Ubuntu" ]; then
  # Install all necessary dependencies
  DEPS="mono-xbuild \
    libmono-microsoft-build-tasks-v4.0-4.0-cil \
    libmono-system-data-datasetextensions4.0-cil
    libmono-system-web-extensions4.0-cil"

  for DEP in $DEPS; do
    if ! dpkg -s $DEP > /dev/null 2>&1; then
      echo "Attempting installation of missing package: $DEP"
      set -x
      sudo apt-get install $DEP
      set +x
    fi
  done
fi

set -x
xbuild Source/Programs/UnrealBuildTool/UnrealBuildTool_Mono.csproj \
  /verbosity:quiet /nologo \
  /p:TargetFrameworkVersion=v4.0 \
  /p:Configuration="Development"

# pass all parameters to UBT
mono Binaries/DotNET/UnrealBuildTool.exe -makefile "$@"