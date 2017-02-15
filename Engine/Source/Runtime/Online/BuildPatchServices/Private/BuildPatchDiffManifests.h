// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class FBuildDiffManifests
{
public:
	static bool DiffManifests(const FString& ManifestFilePathA, const FString& ManifestFilePathB, const FString& OutputFilePath);
};
