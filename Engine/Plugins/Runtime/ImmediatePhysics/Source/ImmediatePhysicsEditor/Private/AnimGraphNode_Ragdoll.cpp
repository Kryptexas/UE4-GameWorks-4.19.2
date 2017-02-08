// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphNode_Ragdoll.h"
#include "CompilerResultsLog.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_Ragdoll

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_Ragdoll::UAnimGraphNode_Ragdoll(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_Ragdoll::GetControllerDescription() const
{
	return LOCTEXT("AnimGraphNode_Ragdoll", "Ragdoll simulation for physics asset");
}

FText UAnimGraphNode_Ragdoll::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_Ragdoll_Tooltip", "This simulates based on the skeletal mesh component's physics asset");
}

FText UAnimGraphNode_Ragdoll::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText(LOCTEXT("AnimGraphNode_Ragdoll_Title", "Ragdoll"));
}

#undef LOCTEXT_NAMESPACE
