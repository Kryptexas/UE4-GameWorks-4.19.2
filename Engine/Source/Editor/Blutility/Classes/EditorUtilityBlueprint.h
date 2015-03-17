// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 * Blueprint for Blutility editor utilities
 */

#pragma once

#include "EditorUtilityBlueprint.generated.h"

UCLASS()
class UEditorUtilityBlueprint : public UBlueprint
{
	GENERATED_BODY()
public:
	UEditorUtilityBlueprint(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

#if WITH_EDITOR
	// UBlueprint interface
	virtual bool SupportedByDefaultBlueprintFactory() const override
	{
		return false;
	}
	// End of UBlueprint interface
#endif
};
