// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetBlueprint.generated.h"

UCLASS(dependson=(UBlueprint, AUserWidget), BlueprintType)
class UMG_API UWidgetBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

	//TODO Add Hierarchy.	
	
#if WITH_EDITOR
	// UBlueprint interface
	virtual bool SupportedByDefaultBlueprintFactory() const OVERRIDE
	{
		return false;
	}
	// End of UBlueprint interface
#endif
};