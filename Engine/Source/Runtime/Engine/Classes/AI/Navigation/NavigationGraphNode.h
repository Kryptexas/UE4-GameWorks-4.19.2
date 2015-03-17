// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"

#include "NavigationGraphNode.generated.h"

UCLASS(config=Engine, MinimalAPI, NotBlueprintable, abstract)
class ANavigationGraphNode : public AActor
{
	GENERATED_BODY()
public:
	ENGINE_API ANavigationGraphNode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
