// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetBlueprint.generated.h"

UCLASS(dependson=(UBlueprint, AUserWidget), BlueprintType)
class UMG_API UWidgetBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

public:
	/** Array of templates for timelines that should be created */
	UPROPERTY()
	TArray<class USlateWrapperComponent*> WidgetTemplates;

	virtual void PostLoad() OVERRIDE;
	virtual void PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph) OVERRIDE;
	
#if WITH_EDITOR
	// UBlueprint interface
	virtual bool SupportedByDefaultBlueprintFactory() const OVERRIDE
	{
		return false;
	}
	// End of UBlueprint interface

	static bool ValidateGeneratedClass(const UClass* InClass);
#endif
};