// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimationGraph.generated.h"

UCLASS(MinimalAPI)
class UAnimationGraph : public UEdGraph
{
	GENERATED_BODY()
public:
	ANIMGRAPH_API UAnimationGraph(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

