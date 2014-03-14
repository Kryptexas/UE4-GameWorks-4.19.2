// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_ModifyBone

UAnimGraphNode_ModifyBone::UAnimGraphNode_ModifyBone(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_ModifyBone::GetControllerDescription() const
{
	return TEXT("Transform (Modify) Bone");
}

FString UAnimGraphNode_ModifyBone::GetKeywords() const
{
	return TEXT("Modify, Transform");
}

FString UAnimGraphNode_ModifyBone::GetTooltip() const
{
	return TEXT("The Transform Bone node alters the transform - i.e. Translation, Rotation, or Scale - of the bone");
}

FString UAnimGraphNode_ModifyBone::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString Result = *GetControllerDescription();
	Result += (TitleType == ENodeTitleType::ListView) ? TEXT(" - ") : TEXT("\n");
	Result += FString::Printf(TEXT("Bone: %s"), *Node.BoneToModify.BoneName.ToString());
	return Result;
}
