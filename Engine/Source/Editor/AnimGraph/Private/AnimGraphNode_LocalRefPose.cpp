// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_LocalRefPose

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_LocalRefPose::UAnimGraphNode_LocalRefPose(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Node.RefPoseType = EIT_LocalSpace;
}

FString UAnimGraphNode_LocalRefPose::GetTooltip() const
{
	return LOCTEXT("UAnimGraphNode_LocalRefPose_Tooltip", "Returns local space reference pose.").ToString();
}

FText UAnimGraphNode_LocalRefPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("UAnimGraphNode_LocalRefPose_Title", "Local Space Ref Pose");
}

#undef LOCTEXT_NAMESPACE
