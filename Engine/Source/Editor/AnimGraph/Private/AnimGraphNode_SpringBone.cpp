// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_SpringBone

UAnimGraphNode_SpringBone::UAnimGraphNode_SpringBone(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_SpringBone::GetControllerDescription() const
{
	return TEXT("Spring controller");
}

FString UAnimGraphNode_SpringBone::GetTooltip() const
{
	return TEXT("The Spring Controller applies a spring solver that can be used to limit how far a bone can stretch from its reference pose position and apply a force in the opposite direction.");
}

FString UAnimGraphNode_SpringBone::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString Result = *GetControllerDescription();
	Result += (TitleType == ENodeTitleType::ListView) ? TEXT(" - ") : TEXT("\n");
	Result += FString::Printf(TEXT("Bone: %s"), *Node.SpringBone.BoneName.ToString());
	return Result;
}
