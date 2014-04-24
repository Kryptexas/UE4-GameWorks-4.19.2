// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_IdentityPose.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_IdentityPose

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_IdentityPose::UAnimGraphNode_IdentityPose(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Node.RefPoseType = EIT_Additive;
}

FString UAnimGraphNode_IdentityPose::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_IdentityPose_Tooltip", "Returns identity pose.").ToString();
}

FText UAnimGraphNode_IdentityPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AnimGraphNode_IdentityPose_Title", "Additive Identity Pose");
}

#undef LOCTEXT_NAMESPACE
