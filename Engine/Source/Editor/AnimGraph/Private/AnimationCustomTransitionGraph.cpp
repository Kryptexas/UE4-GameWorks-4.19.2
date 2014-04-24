// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"


#define LOCTEXT_NAMESPACE "AnimationCustomTransitionGraph"

/////////////////////////////////////////////////////
// UAnimationStateGraph

UAnimationCustomTransitionGraph::UAnimationCustomTransitionGraph(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UAnimGraphNode_CustomTransitionResult* UAnimationCustomTransitionGraph::GetResultNode()
{
	return MyResultNode;
}

#undef LOCTEXT_NAMESPACE
