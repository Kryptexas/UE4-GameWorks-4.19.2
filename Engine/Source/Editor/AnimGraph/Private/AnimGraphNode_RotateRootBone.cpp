// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "AnimGraphNode_RotateRootBone.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_RotateRootBone

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_RotateRootBone::UAnimGraphNode_RotateRootBone(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FLinearColor UAnimGraphNode_RotateRootBone::GetNodeTitleColor() const
{
	return FLinearColor(0.7f, 0.7f, 0.7f);
}

FString UAnimGraphNode_RotateRootBone::GetTooltip() const
{
	return LOCTEXT("RotateRootBone", "Rotate Root Bone").ToString();
}

FText UAnimGraphNode_RotateRootBone::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("RotateRootBone", "Rotate Root Bone");
}

FString UAnimGraphNode_RotateRootBone::GetNodeCategory() const
{
	return TEXT("Tools");
}

#undef LOCTEXT_NAMESPACE
