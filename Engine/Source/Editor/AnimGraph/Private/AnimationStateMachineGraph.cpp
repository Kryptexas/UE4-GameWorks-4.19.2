// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"


#define LOCTEXT_NAMESPACE "AnimationGraph"

/////////////////////////////////////////////////////
// UAnimationStateMachineGraph

UAnimationStateMachineGraph::UAnimationStateMachineGraph(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bAllowDeletion = false;
	bAllowRenaming = true;
}


#undef LOCTEXT_NAMESPACE
