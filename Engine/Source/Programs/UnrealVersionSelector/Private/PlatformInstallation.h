// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealVersionSelector.h"

#if PLATFORM_WINDOWS
	#include "Windows/WindowsPlatformInstallation.h"
	typedef FWindowsPlatformInstallation FPlatformInstallation;
#else
	#include "GenericPlatform/GenericPlatformInstallation.h"
	typedef FGenericPlatformInstallation FPlatformInstallation;
#endif

// Utility functions
int32 ParseReleaseVersion(const FString &Version);

bool GetDefaultEngineId(FString &OutId);
bool GetDefaultEngineRootDir(FString &OutId);

bool GetEngineRootDirFromId(const FString &Id, FString &OutRootDir);
bool GetEngineIdFromRootDir(const FString &RootDir, FString &OutId);
