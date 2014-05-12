Build scripts for native Linux build
====================================

This document describes how to build UE4 natively on a Linux host.
So far this has only been tested on Debian/Sid and Ubuntu/Precise.

A wiki page page regarding building on linux is also maintained at
https://wiki.unrealengine.com/Building_On_Linux, and public IRC discussion
happens on #UE4Linux on freenode.

Prerequisites
-------------

You will need mono + gmcs and several other packages installed in order to build
EU4. The command to install these on Ubundu 12.04 (Precise) is:

    $ apt-get install mono-mcs mono-xbuild dos2unix clang-3.3 \
        libogg-dev libmono-microsoft-build-tasks-v4.0-4.0-cil \
        libmono-system-data-datasetextensions4.0-cil \
        libmono-system-web-extensions4.0-cil \
        libmono-system-management4.0-cil

Building
--------

1. Download the additional .zip archive from the github releases page.

2. Download v129 of the Steamworks SDK.

3. Install the downloaded dependencies with:

        $ ./UpdateDeps.sh.

4. Build third party libraries:

        $ ./BuildThirdParty.sh.

5. Build UnrealBuildTool (UBT) and generate top level Makefile:

        $ ./GenerateProjectFiles.sh

6. Build your targets using the top-level Makefile. e.g:

        $ make UE4Client

   Or by running `Build.sh` directly:

        $ Engine/Build/BatchFiles/Linux/Build.sh UE4Client Linux Debug
