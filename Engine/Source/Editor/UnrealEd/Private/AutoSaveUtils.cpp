// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AutoSaveUtils.h"
#include "Misc/Paths.h"

FString AutoSaveUtils::GetAutoSaveDir()
{
	return FPaths::ProjectSavedDir() / TEXT("Autosaves");
}
