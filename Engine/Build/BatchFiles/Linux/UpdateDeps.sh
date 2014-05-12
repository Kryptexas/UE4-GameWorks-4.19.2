#!/bin/bash
# This script will verify the downloaded zip file depenencies, unzip them and
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

FILES="Optional.zip Required_1of3.zip Required_2of3.zip Required_3of3.zip"
REQUIRED_SHA1SUMS=$SCRIPT_DIR/dependencies.sha1sums
INSTALLED_SHA1SUMS=$REQUIRED_SHA1SUMS.installed

PERFORCE_ARCHIVE=$ARCHIVE_ROOT/p4api.tgz
STEAM_ARCHIVE=$ARCHIVE_ROOT/steamworks_sdk_129.zip
FBX_ARCHIVE=$ARCHIVE_ROOT/fbx20142_1_fbxsdk_linux.tar.gz

CheckInstalled() {
  echo "==> Checking existing install"
  # Phase one, check sha1 sums of previously installed files.
  if [ -f $INSTALLED_SHA1SUMS ]; then
    if [ "`cat $INSTALLED_SHA1SUMS`" = "`cat $REQUIRED_SHA1SUMS`" ]; then
      echo "Dependencies are up-to-date."
      echo "(remove $INSTALLED_SHA1SUMS file and re-run to force install)."
      exit 0
    else
      echo "Installed dependencies are out-of-date."
    fi
  else
    echo "No installed dependenies found."
  fi
}

Download() {
  # For those archives that are missing some can be downloaded automatically using curl
  # The steamworks SDK lives on a standard public URL but seems to be authorised based
  # on source IP for a certain amount of time, afterwhich we get a 401 error.
  #if [ ! -f $STEAM_ARCHIVE ]; then
  #  echo "==> Downloading steamworks SDK"
  #  curl --progress -o $STEAM_ARCHIVE https://partner.steamgames.com/downloads/steamworks_sdk_129.zip
  #fi
  if [ ! -f $PERFORCE_ARCHIVE ]; then
	echo "==> Downloading p4api.tgz"
    curl --progress -o $PERFORCE_ARCHIVE http://ftp.perforce.com/perforce/r12.1/bin.linux26x86_64/p4api.tgz
  fi
  if [ ! -f $FBX_ARCHIVE ]; then
	echo "==> Downloading FBX SDK"
	curl --progress -o $FBX_ARCHIVE http://images.autodesk.com/adsk/files/fbx20142_1_fbxsdk_linux.tar.gz
  fi
}

VerifyDownloads() {
  echo "==> Verifying downloaded archives"
  cd $ARCHIVE_ROOT
  if ! sha1sum -c $REQUIRED_SHA1SUMS; then
	echo ""
    echo "ERROR: One or more archives files are missing or out-of-date."
	echo "Please download zip files from github release page and place them in "
	echo "'$ARCHIVE_ROOT'."
	echo "If you have already downloaded them somewhere else then you can pass "
	echo "the location as the first argument to this script."
    exit 1
  fi
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
  for file in $FILES; do
    file_full=$ARCHIVE_ROOT/$file
    if [ ! -f $file_full ]; then
      echo "Zip file dependency not found: $file_full"
      exit 1
    fi
    echo "Extracting $file_full"
    unzip -q $file_full
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

  cd ICU/icu4c-51_2/source/
  chmod +x configure
  chmod +x config.sub
  chmod +x config.guess
  cd -

  patch -p1 < ${SCRIPT_DIR}/hlslcc.patch
  patch -p1 < ${SCRIPT_DIR}/nvtesslib.patch
  patch -p1 < ${SCRIPT_DIR}/HACD.patch
  patch -p1 < ${SCRIPT_DIR}/mcpp.patch
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
  rm -rf Engine/Source/ThirdParty/Steamworks/Steamv129
  mkdir Engine/Source/ThirdParty/Steamworks/Steamv129
  cd Engine/Source/ThirdParty/Steamworks/Steamv129
  unzip -q $STEAM_ARCHIVE
  cd ${TOP_DIR}
  mkdir -p Engine/Binaries/Linux/
  rm -f Engine/Binaries/Linux/libsteam_api.so
  ln -s Engine/Source/ThirdParty/Steamworks/Steamv129/sdk/redistributable_bin/linux64/libsteam_api.so Engine/Binaries/Linux/
}

InstallFBX() {
  echo "==> Installing FBX"
  cd Engine/Source/ThirdParty/FBX
  tar xf $FBX_ARCHIVE
  rm -rf fbx
  mkdir fbx
  yes yes | ./fbx20142_1_fbxsdk_linux fbx > /dev/null
  rm -rf 2014.2.1/lib/linux
  mkdir 2014.2.1/lib/linux
  cp fbx/lib/gcc4/x64/release/libfbxsdk.a 2014.2.1/lib/linux
  cd ${TOP_DIR}
}

UpdateInstallStamp() {
  echo "==> Updating install stamp ($INSTALLED_SHA1SUMS)"
  cp $REQUIRED_SHA1SUMS $INSTALLED_SHA1SUMS
}

CheckInstalled
Download
VerifyDownloads
ExtractArchives
ApplyPatches
InstallFBX
InstallPerforce
InstallSteamworks
UpdateInstallStamp

echo "********** SUCCESS ****************"
