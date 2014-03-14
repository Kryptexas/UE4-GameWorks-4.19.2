// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_CustomTransitionResult

UAnimGraphNode_CustomTransitionResult::UAnimGraphNode_CustomTransitionResult(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FLinearColor UAnimGraphNode_CustomTransitionResult::GetNodeTitleColor() const
{
	return FLinearColor(1.0f, 0.65f, 0.4f);
}

FString UAnimGraphNode_CustomTransitionResult::GetTooltip() const
{
	return TEXT("Result node for a custom transition blend graph");
}

FString UAnimGraphNode_CustomTransitionResult::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return TEXT("Custom Transition Blend Result");
}
