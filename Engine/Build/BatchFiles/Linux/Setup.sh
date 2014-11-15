#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "$BASH_SOURCE")" ; pwd)

set -e

TOP_DIR=$(cd $SCRIPT_DIR/../../.. ; pwd)
cd ${TOP_DIR}

IS_GITHUB_BUILD=true
if [ -e Build/PerforceBuild.txt ]; then
  IS_GITHUB_BUILD=false
fi

# Ubuntu/Debian/Mint
if [ "$(lsb_release --id)" = "Distributor ID:	Ubuntu" -o "$(lsb_release --id)" = "Distributor ID:	Debian" -o "$(lsb_release --id)" = "Distributor ID:	LinuxMint" ]; then
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
	mono Binaries/DotNET/GitDependencies.exe --prompt "$@"
	set -e

	#echo Unpacking and massaging the files
	#pushd Build/BatchFiles/Linux > /dev/null
	#./UpdateDeps.sh 
	#popd > /dev/null
else
	echo Perforce build
	echo Assuming availability of up to date third-party libraries
fi

echo
pushd Build/BatchFiles/Linux > /dev/null
./BuildThirdParty.sh
popd > /dev/null

touch Build/OneTimeSetupPerformed



