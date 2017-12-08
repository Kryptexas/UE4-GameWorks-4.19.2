// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Integrate Flex actions into the level editor context menu
class FFlexLevelEditorMenuExtensions
{
public:
	static void InstallHooks();
	static void RemoveHooks();
};
