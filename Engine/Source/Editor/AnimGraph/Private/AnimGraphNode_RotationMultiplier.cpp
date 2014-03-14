// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_RotationMultiplier

UAnimGraphNode_RotationMultiplier::UAnimGraphNode_RotationMultiplier(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_RotationMultiplier::GetControllerDescription() const
{
	return TEXT("Apply a percentage of Rotation");
}

FString UAnimGraphNode_RotationMultiplier::GetTooltip() const
{
	return TEXT("The Apply a Percentage of Rotation control drives the Rotation of a target bone at some specified percentage of the Rotation of another bone within the Skeleton.");
}

FString UAnimGraphNode_RotationMultiplier::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString Result = *GetControllerDescription();
	Result += (TitleType == ENodeTitleType::ListView) ? TEXT(" - ") : TEXT("\n");
	Result += FString::Printf(TEXT("Bone: %s"), *Node.TargetBone.BoneName.ToString());
	return Result;
}
