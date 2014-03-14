// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavigationGraphNodeComponent.generated.h"

UCLASS(HeaderGroup=Component, config=Engine, dependson=ANavigationGraph, MinimalAPI, HideCategories=(Mobility))
class UNavigationGraphNodeComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FNavGraphNode Node;

	UPROPERTY()
	UNavigationGraphNodeComponent* NextNodeComponent;

	UPROPERTY()
	UNavigationGraphNodeComponent* PrevNodeComponent;

	// Begin UObject interface.
	virtual void BeginDestroy() OVERRIDE;
};
