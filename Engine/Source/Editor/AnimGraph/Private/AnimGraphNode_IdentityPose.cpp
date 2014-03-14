// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_IdentityPose

UAnimGraphNode_IdentityPose::UAnimGraphNode_IdentityPose(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Node.RefPoseType = EIT_Additive;
}

FString UAnimGraphNode_IdentityPose::GetTooltip() const
{
	return TEXT("Returns identity pose.");
}

FString UAnimGraphNode_IdentityPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(TEXT("Additive Identity Pose"));
}

