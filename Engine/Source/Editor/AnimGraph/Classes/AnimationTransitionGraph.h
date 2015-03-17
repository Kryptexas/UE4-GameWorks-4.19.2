// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimationGraph.h"
#include "AnimationTransitionGraph.generated.h"

UCLASS(MinimalAPI)
class UAnimationTransitionGraph : public UAnimationGraph
{
	GENERATED_BODY()
public:
	ANIMGRAPH_API UAnimationTransitionGraph(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY()
	class UAnimGraphNode_TransitionResult* MyResultNode;

	ANIMGRAPH_API class UAnimGraphNode_TransitionResult* GetResultNode();
};

