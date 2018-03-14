#!/bin/sh
# Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#
# Simple wrapper around GenerateProjectFiles.sh using the
# .command extension enables it to be run from the OSX Finder.

sh "`dirname "$0"`"/GenerateProjectFiles.sh $*
