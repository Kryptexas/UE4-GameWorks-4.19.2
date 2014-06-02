// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetActor.generated.h"

/** A widget actor can be added to the world, giving a widget 3D world position. */
UCLASS()
class UMG_API AWidgetActor : public AActor
{
	GENERATED_UCLASS_BODY()

	// AActor interface
	//virtual void OnConstruction(const FTransform& Transform);
	//virtual void Destroyed();
	//virtual void RerunConstructionScripts();
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction);
	// End of AActor interface
};
