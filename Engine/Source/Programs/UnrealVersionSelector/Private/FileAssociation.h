// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealVersionSelector.h"

bool SetEngineIdForProject(const FString &ProjectFileName, const FString &Id);
bool GetEngineIdForProject(const FString &ProjectFileName, FString &OutId);

bool GetEngineRootDirForProject(const FString &ProjectFileName, FString &OutDirName);
