// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavigationGraphNode.generated.h"

UCLASS(config=Engine, dependson=UNavigationSystem, MinimalAPI, NotBlueprintable, abstract)
class ANavigationGraphNode : public AActor
{
	GENERATED_UCLASS_BODY()
};
