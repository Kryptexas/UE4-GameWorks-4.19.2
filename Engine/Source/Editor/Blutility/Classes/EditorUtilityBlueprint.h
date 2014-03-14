// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Blueprint for Blutility editor utilities
 */

#pragma once

#include "EditorUtilityBlueprint.generated.h"

UCLASS()
class UEditorUtilityBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// UBlueprint interface
	virtual bool SupportedByDefaultBlueprintFactory() const OVERRIDE
	{
		return false;
	}
	// End of UBlueprint interface
#endif
};
