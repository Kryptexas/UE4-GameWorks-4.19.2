// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimationGraph.h"
#include "AnimationCustomTransitionGraph.generated.h"

UCLASS(MinimalAPI)
class UAnimationCustomTransitionGraph : public UAnimationGraph
{
	GENERATED_BODY()
public:
	ANIMGRAPH_API UAnimationCustomTransitionGraph(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Result node within the state's animation graph
	UPROPERTY()
	class UAnimGraphNode_CustomTransitionResult* MyResultNode;

	//@TODO: Document
	ANIMGRAPH_API class UAnimGraphNode_CustomTransitionResult* GetResultNode();
};

