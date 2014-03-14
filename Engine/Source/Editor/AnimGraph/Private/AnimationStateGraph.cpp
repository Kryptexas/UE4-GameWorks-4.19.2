// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"


#define LOCTEXT_NAMESPACE "AnimationStateGraph"

/////////////////////////////////////////////////////
// UAnimationStateGraph

UAnimationStateGraph::UAnimationStateGraph(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UAnimGraphNode_StateResult* UAnimationStateGraph::GetResultNode()
{
	return MyResultNode;
}

#undef LOCTEXT_NAMESPACE
