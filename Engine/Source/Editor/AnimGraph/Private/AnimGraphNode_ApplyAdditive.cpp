// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "AnimGraphNode_ApplyAdditive.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_ApplyAdditive

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_ApplyAdditive::UAnimGraphNode_ApplyAdditive(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FLinearColor UAnimGraphNode_ApplyAdditive::GetNodeTitleColor() const
{
	return FLinearColor(0.75f, 0.75f, 0.75f);
}

FString UAnimGraphNode_ApplyAdditive::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_ApplyAdditive_Tooltip", "Apply additive animation to normal pose").ToString();
}

FText UAnimGraphNode_ApplyAdditive::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AnimGraphNode_ApplyAdditive_Title", "Apply Additive");
}

FString UAnimGraphNode_ApplyAdditive::GetNodeCategory() const
{
	return TEXT("Blends");
	//@TODO: TEXT("Apply additive to normal pose"), TEXT("Apply additive pose"));
}

#undef LOCTEXT_NAMESPACE
