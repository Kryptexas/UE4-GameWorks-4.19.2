// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"


#define LOCTEXT_NAMESPACE "AnimationStateGraph"

/////////////////////////////////////////////////////
// UAnimationTransitionGraph

UAnimationTransitionGraph::UAnimationTransitionGraph(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UAnimGraphNode_TransitionResult* UAnimationTransitionGraph::GetResultNode()
{
	return MyResultNode;
}

#undef LOCTEXT_NAMESPACE
