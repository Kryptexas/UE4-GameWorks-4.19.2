#!/bin/bash
# This script will verify the downloaded zip file dependencies, unzip them and
# apply linux-specific patches.  It tries to clean up any previous versions
# using 'git clean' on certain directories.
# WARNING: This script can be destructive so use with caution if you have
# unsaved local changes.
#
# This script does not handle building of any of the ThirdParty dependencies.
# (See BuildThirdParty.sh).

# Location of downloaded .zip files.
if [ -n "$1" ]; then
  ARCHIVE_ROOT=$1
else
  ARCHIVE_ROOT=$HOME/Downloads
fi


if [ ! -d $ARCHIVE_ROOT ]; then
  echo "Download root not found: $ARCHIVE_ROOT"
  exit 1
fi

# Zip file dependencies.

# Make sure that the current working directory is the root the git checkout,
# which is four levels up from the this script.
SCRIPT_DIR=$(cd "$(dirname "$BASH_SOURCE")" ; pwd)
TOP_DIR=$(cd $SCRIPT_DIR/../../../.. ; pwd)
cd ${TOP_DIR}
set -e
trap "echo '==> An error occured while running UpdateDeps.sh'" ERR

DEPENDENCIES=$SCRIPT_DIR/dependencies.txt
INSTALLED_DEPENDENCIES=${DEPENDENCIES%.txt}.installed

#TODO: Move these hardcodings out of this file
PERFORCE_ARCHIVE=$ARCHIVE_ROOT/p4api.tgz
STEAM_ARCHIVE=$ARCHIVE_ROOT/steamworks_sdk_129a.zip

# Make it super easy for users of certain linux distributions.
CheckDistroDependencies() {
  echo "==> Checking build dependencies"
  if [ "$(lsb_release --id)" = "Distributor ID:	Ubuntu" -o "$(lsb_release --id)" = "Distributor ID:	Debian" ]; then
    # Install all necessary dependencies
    DEPS="libqt4-dev
      libglew-dev
      dos2unix 
      clang-3.3"

    for DEP in $DEPS; do
      if ! dpkg -s $DEP > /dev/null 2>&1; then
        echo "Attempting installation of missing package: $DEP"
        set -x
        sudo apt-get install $DEP
        set +x
      fi
    done
  fi
}

CheckInstalled() {
  echo "==> Checking existing install"
  # Phase one, check sha1 sums of previously installed files.
  if [ -f $INSTALLED_DEPENDENCIES ]; then
    if [ "`cat $INSTALLED_DEPENDENCIES`" = "`cat $DEPENDENCIES`" ]; then
      echo "Dependencies are up-to-date."
      echo "(remove $INSTALLED_DEPENDENCIES file and re-run to force install)."
      exit 0
    else
      echo "Installed dependencies are out-of-date."
    fi
  else
    echo "No installed dependenies found."
  fi
}

Download() {
  # For those archives that are missing some can be downloaded automatically
  # using curl.
  cat $DEPENDENCIES | while read line; do
    DEP_URL=`echo $line | cut -d " " -f 2`
    DEP_WALLED=`echo $line | cut -d " " -f 3`
    DEP_FILE=$(basename $DEP_URL)
    if [ ! -f $ARCHIVE_ROOT/$DEP_FILE ]; then
      if [ "$DEP_WALLED" -eq 0 ]; then
        echo "==> Downloading $DEP_FILE"
        curl --progress -o $ARCHIVE_ROOT/$DEP_FILE $DEP_URL
      fi
    fi
  done
}

VerifyDownloads() {
  echo "==> Verifying downloaded archives"
  cd $ARCHIVE_ROOT
  cat $DEPENDENCIES | while read line; do
    DEP_HASH=`echo $line | cut -d " " -f 1`
    DEP_URL=`echo $line | cut -d " " -f 2`
    DEP_FILE=$(basename $DEP_URL)
    if ! echo "$DEP_HASH  $DEP_FILE" | sha1sum -c -; then
      echo ""
      echo "ERROR: Missing or out-of-date dependency: $DEP_FILE"
      echo "Please download the following file to $ARCHIVE_ROOT:"
      echo "  $DEP_URL"
      echo "If you have downloaded these files somewhere else you can"
      echo "pass the location as the first argument to this script."
      exit 1
    fi
  done

  cd ${TOP_DIR}
}

ExtractArchives() {
  # Clean any files installed from .zip files.
  echo "==> Cleaning existing dependencies"
  echo "About to cleanup previous zip contents with git clean."
  echo "WARNING: This operation can be destructive: It removes any untracked"
  echo "files in parts of the tree (e.g. Source/ThirdParty)."
  while true; do
    read -p "Do you want to continue? (y/n) " yn
    case $yn in
      [Yy] ) break;;
      [Nn] ) exit;;
      * ) echo "Please answer y or n.";;
    esac
  done

  CLEAN_DIRS="
    Engine/Binaries
    Engine/Source/ThirdParty
    Engine/Plugins/Script/ScriptGeneratorPlugin/Binaries
    Engine/Plugins/Messaging/UdpMessaging/Binaries
    Engine/Content
    Engine/Documentation
    Engine/Extras
    Samples/StarterContent
    Templates"
  for dir in $CLEAN_DIRS; do
    git clean -xfd $dir
  done

  echo "==> Installing dependencies"
  # Extract files and write new sha1 files.
  cat $DEPENDENCIES | while read line; do
    DEP_URL=`echo $line | cut -d " " -f 2`
    if [[ $(dirname $DEP_URL) != https://github.com/EpicGames/* ]]; then
      continue
    fi
    DEP_FILE=$(basename $DEP_URL)
    file_full=$ARCHIVE_ROOT/$DEP_FILE
    if [ ! -f $file_full ]; then
      echo "Zip file dependency not found: $file_full"
      exit 1
    fi
    echo "Extracting $file_full"
    unzip -q -n $file_full
  done
}

ApplyPatches() {
  echo "==> Applying patches"
  cd Engine/Source/ThirdParty
  echo "PWD=$PWD"

  cd libOpus/opus-1.1
  dos2unix missing
  dos2unix depcomp
  dos2unix ltmain.sh
  dos2unix config.*
  dos2unix package_version
  dos2unix configure.ac
  dos2unix `find -name "*.in"`
  dos2unix `find -name "*.m4"`
  autoconf
  cd -

  cd MCPP/mcpp-2.7.2/
  chmod +x configure
  dos2unix configure
  dos2unix src/config.h.in config/*
  cd -

  cd hlslcc/hlslcc
  dos2unix src/hlslcc_lib/hlslcc.h
  dos2unix src/hlslcc_lib/hlslcc_private.h
  dos2unix src/hlslcc_lib/glsl/ir_gen_glsl.h
  cd -

  cd Vorbis/libvorbis-1.3.2/
  dos2unix configure
  dos2unix missing
  dos2unix ltmain.sh
  dos2unix depcomp
  dos2unix config.*
  dos2unix `find -name Makefile.in`
  chmod +x configure
  chmod +x config.sub
  chmod +x config.guess
  cd -

  cd Ogg/libogg-1.2.2/
  dos2unix -f configure
  dos2unix missing
  dos2unix ltmain.sh
  dos2unix depcomp
  dos2unix config.*
  dos2unix `find -name Makefile.in`
  chmod +x configure
  chmod +x config.sub
  chmod +x config.guess
  cd -

  cd SDL2/
  pwd
  tar xf SDL2-2.0.3/build/SDL2-2.0.3.tar.gz -C SDL2-2.0.3/build
  rm -f DisplayServerExtensions/bin/libdsext.a
  patch -f -p1 < ${SCRIPT_DIR}/SDL2.patch
  cd -

  # This needs to be moved out of the way as the LinuxToolChain.cs
  # Links via -l and not via path name so will pick the .so instead
  cd FBX/2014.2.1/
  pwd
  rm -f lib/linux/x86_64-unknown-linux-gnu/libfbxsdk.so
  cd -

  cd ${TOP_DIR}
}

RenameContent() {
  echo "==> Correcting Filename case"
  cd Engine/Content
  echo "PWD=$PWD"

  find . -name "*.PNG" -exec rename 's/\.PNG$/\.png/' {} \;

  cd ${TOP_DIR}
}

InstallPerforce() {
  echo "==> Installing Perforce (p4api) libraries"
  cd Engine/Source/ThirdParty/Perforce
  rm -rf p4api-2012.1.442152 p4api-2012.1/lib/linux
  tar xf $PERFORCE_ARCHIVE
  cp -r p4api-2012.1.442152/lib p4api-2012.1/lib/linux
  cd ${TOP_DIR}
}

InstallSteamworks() {
  echo "==> Installing Steamworks SDK"
  rm -rf Engine/Source/ThirdParty/Steamworks/Steamv129a
  mkdir Engine/Source/ThirdParty/Steamworks/Steamv129a
  cd Engine/Source/ThirdParty/Steamworks/Steamv129a
  unzip -q $STEAM_ARCHIVE
  cd ${TOP_DIR}
  mkdir -p Engine/Binaries/Linux/
  rm -f Engine/Binaries/Linux/libsteam_api.so
  cd Engine/Binaries/Linux
  ln -s ../../Source/ThirdParty/Steamworks/Steamv129a/sdk/redistributable_bin/linux64/libsteam_api.so
  cd ${TOP_DIR}
}

UpdateInstallStamp() {
  echo "==> Updating install stamp ($INSTALLED_DEPENDENCIES)"
  cp $DEPENDENCIES $INSTALLED_DEPENDENCIES
}

CheckDistroDependencies
CheckInstalled
Download
VerifyDownloads
ExtractArchives
ApplyPatches
RenameContent
InstallPerforce
InstallSteamworks
UpdateInstallStamp

echo "********** SUCCESS ****************"

# Once we've successfully updated the deps we need to rebuild the
# ThirdParty modules.  This is mostly so that we get everything built
# with -fPIC.  BuildThirdParty.sh is a seperate script so that it can
# be run repeatedly and independently of UpdateDeps.sh if needed.
# $SCRIPT_DIR/BuildThirdParty.sh
