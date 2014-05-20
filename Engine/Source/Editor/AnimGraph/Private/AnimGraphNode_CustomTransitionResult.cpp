// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_CustomTransitionResult.h"
#include "GraphEditorSettings.h"


/////////////////////////////////////////////////////
// UAnimGraphNode_CustomTransitionResult

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_CustomTransitionResult::UAnimGraphNode_CustomTransitionResult(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FLinearColor UAnimGraphNode_CustomTransitionResult::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->ResultNodeTitleColor;
}

FString UAnimGraphNode_CustomTransitionResult::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_CustomTransitionResult_Tooltip", "Result node for a custom transition blend graph").ToString();
}

FText UAnimGraphNode_CustomTransitionResult::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AnimGraphNode_CustomTransitionResult_Title", "Custom Transition Blend Result");
}

#undef LOCTEXT_NAMESPACE