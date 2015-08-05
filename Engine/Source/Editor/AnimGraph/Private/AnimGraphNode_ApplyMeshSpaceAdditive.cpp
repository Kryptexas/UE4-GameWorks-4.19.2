// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "AnimGraphNode_ApplyMeshSpaceAdditive.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_ApplyAdditive

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_ApplyMeshSpaceAdditive::UAnimGraphNode_ApplyMeshSpaceAdditive(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FLinearColor UAnimGraphNode_ApplyMeshSpaceAdditive::GetNodeTitleColor() const
{
	return FLinearColor(0.75f, 0.75f, 0.75f);
}

FText UAnimGraphNode_ApplyMeshSpaceAdditive::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_ApplyMeshSpaceAdditive_Tooltip", "Apply mesh space additive animation to normal pose");
}

FText UAnimGraphNode_ApplyMeshSpaceAdditive::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AnimGraphNode_ApplyMeshSpaceAdditive_Title", "Apply Mesh Space Additive");
}

FString UAnimGraphNode_ApplyMeshSpaceAdditive::GetNodeCategory() const
{
	return TEXT("Blends");
	//@TODO: TEXT("Apply additive to normal pose"), TEXT("Apply additive pose"));
}

#undef LOCTEXT_NAMESPACE
