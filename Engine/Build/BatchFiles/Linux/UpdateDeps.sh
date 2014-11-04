#!/bin/bash
# This script will unzip the downloaded archives and convert to Unix format where needed.  
# It tries to clean up any previous versions using 'git clean' on certain directories.
# WARNING: This script can be destructive so use with caution if you have
# unsaved local changes.
#
# This script does not handle building of any of the ThirdParty dependencies.
# (See BuildThirdParty.sh).

# Location of the archive files (shared with GetAssets.py)
if [ -z $ARCHIVE_ROOT ]; then
  echo You should have ARCHIVE_ROOT variable set to the same location as used in GetAssets.py. If you don't know what this is about, don't invoke this file directly.
fi

# Prefix of the archive files (should match one used by GetAssets.py)
if [ -z $GITHUB_TAG ]; then
  echo You should have GITHUB_TAG variable set to the same release tag as used in GetAssets.py. If you don't know what this is about, don't invoke this file directly.
fi

if [ ! -d $ARCHIVE_ROOT ]; then
  echo "Download root not found: $ARCHIVE_ROOT"
  exit 1
fi

# Zipped archives.
ARCHIVES="$GITHUB_TAG-Required_1of3.zip
    $GITHUB_TAG-Required_2of3.zip
    $GITHUB_TAG-Required_3of3.zip
    $GITHUB_TAG-Optional.zip"

# Make sure that the current working directory is the root the git checkout,
# which is four levels up from the this script.
SCRIPT_DIR=$(cd "$(dirname "$BASH_SOURCE")" ; pwd)
TOP_DIR=$(cd $SCRIPT_DIR/../../../.. ; pwd)
cd ${TOP_DIR}
set -e
trap "echo '==> An error occured while running UpdateDeps.sh'" ERR

CleanRepo() 
{
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
  for Dir in $CLEAN_DIRS; do
    git clean -xfd $Dir
  done
}

ExtractArchives()
{
  echo "==> Installing dependencies"
  for Archive in $ARCHIVES; do
    FullPathArchive=$ARCHIVE_ROOT/$Archive
    if [ ! -f $FullPathArchive ]; then
      echo "Zip file dependency not found: $FullPathArchive"
      exit 1
    fi
    echo "Extracting $FullPathArchive"
    unzip -q -n $FullPathArchive
  done
}

Unixify()
{
  echo "==> Converting to unix line endings"
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
  cd -

  cd Ogg/libogg-1.2.2/
  dos2unix -f configure
  dos2unix missing
  dos2unix ltmain.sh
  dos2unix depcomp
  dos2unix config.*
  dos2unix `find -name Makefile.in`
  cd -

  cd ${TOP_DIR}
}

CleanRepo
ExtractArchives
Unixify

echo "********** SUCCESS ****************"

