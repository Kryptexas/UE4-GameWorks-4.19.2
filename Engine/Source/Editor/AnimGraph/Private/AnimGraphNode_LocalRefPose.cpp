// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_LocalRefPose

UAnimGraphNode_LocalRefPose::UAnimGraphNode_LocalRefPose(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Node.RefPoseType = EIT_LocalSpace;
}

FString UAnimGraphNode_LocalRefPose::GetTooltip() const
{
	return TEXT("Returns local space reference pose.");
}

FString UAnimGraphNode_LocalRefPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(TEXT("Local Space Ref Pose"));
}

