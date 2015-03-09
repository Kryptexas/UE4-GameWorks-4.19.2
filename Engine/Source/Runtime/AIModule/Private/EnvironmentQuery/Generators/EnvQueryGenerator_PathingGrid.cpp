// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_PathingGrid.h"

UEnvQueryGenerator_PathingGrid::UEnvQueryGenerator_PathingGrid(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MaxDistance.DefaultValue = 100.0f;
	SpaceBetween.DefaultValue = 10.0f;
	PathToItem.DefaultValue = true;
}

void UEnvQueryGenerator_PathingGrid::PostLoad()
{
	Super::PostLoad();

	GridSize = MaxDistance;
}
