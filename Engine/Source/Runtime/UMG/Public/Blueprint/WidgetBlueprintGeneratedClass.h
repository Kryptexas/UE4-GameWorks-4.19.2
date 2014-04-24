// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetBlueprintGeneratedClass.generated.h"

UCLASS(dependson=(UWidgetBlueprint))
class UMG_API UWidgetBlueprintGeneratedClass : public UBlueprintGeneratedClass
{
	GENERATED_UCLASS_BODY()

	/** A tree of the widget templates to be created */
	UPROPERTY()
	class UWidgetTree* WidgetTree;

	virtual void CreateComponentsForActor(AActor* Actor) const OVERRIDE;
};