Build scripts for native Linux build
====================================

This document describes how to build UE4 natively on a Linux host.
So far this has only been tested on Debian/Sid and Ubuntu/Precise.

Prerequisites
-------------

You will need mono + gmcs installed in order to build:

    $ apt-get install gmcs

Building
--------

1. Install all three additional .zip archive, including `Optional.zip`.
   `Optional.zip` is needed since it provides the Linux binaries for
   PhysX.

2. Build third party libraries:

        $ ./BuildThirdParty.sh.

3. Build UnrealBuildTool (UBT) und generate top level Makefile:

        $ ./GenerateProjectFiles.sh

4. Build your targets using the top-level Makefile. e.g:

        $ make UE4Client

   Or by running `Build.sh` directly:

        $ Engine/Build/BatchFiles/Linux/Build.sh UE4Client Linux Debug