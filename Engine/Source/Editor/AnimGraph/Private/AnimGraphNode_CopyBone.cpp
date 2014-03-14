// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_CopyBoneSkeletalControl

UAnimGraphNode_CopyBone::UAnimGraphNode_CopyBone(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_CopyBone::GetControllerDescription() const
{
	return TEXT("Copy Bone");
}

FString UAnimGraphNode_CopyBone::GetTooltip() const
{
	return TEXT("The Copy Bone control copies the Transform data or any component of it - i.e. Translation, Rotation, or Scale - from one bone to another.");
}

FString UAnimGraphNode_CopyBone::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(TEXT("%s\nSource Bone: %s\nTarget Bone: %s"), *GetControllerDescription(), *Node.SourceBone.BoneName.ToString(), *Node.TargetBone.BoneName.ToString());
}
