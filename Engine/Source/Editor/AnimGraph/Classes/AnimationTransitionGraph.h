// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimationTransitionGraph.generated.h"

UCLASS(MinimalAPI)
class UAnimationTransitionGraph : public UAnimationGraph
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UAnimGraphNode_TransitionResult* MyResultNode;

	ANIMGRAPH_API class UAnimGraphNode_TransitionResult* GetResultNode();
};

