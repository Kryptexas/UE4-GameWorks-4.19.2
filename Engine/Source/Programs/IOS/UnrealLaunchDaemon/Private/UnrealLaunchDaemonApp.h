// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "UnrealLaunchDaemon.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLaunchDaemon, Log, All);

/**
 * Run the UnrealLaunchDaemon 
 */
void RunUnrealLaunchDaemon(const TCHAR* Commandline);
