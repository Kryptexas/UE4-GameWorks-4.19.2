// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_RotationMultiplier

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_RotationMultiplier::UAnimGraphNode_RotationMultiplier(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FText UAnimGraphNode_RotationMultiplier::GetControllerDescription() const
{
	return LOCTEXT("ApplyPercentageOfRotation", "Apply a percentage of Rotation");
}

FString UAnimGraphNode_RotationMultiplier::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_RotationMultiplier_Tooltip", "The Apply a Percentage of Rotation control drives the Rotation of a target bone at some specified percentage of the Rotation of another bone within the Skeleton.").ToString();
}

FText UAnimGraphNode_RotationMultiplier::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
	Args.Add(TEXT("BoneName"), FText::FromName(Node.TargetBone.BoneName));

	if(TitleType == ENodeTitleType::ListView)
	{
		return FText::Format(LOCTEXT("AnimGraphNode_ModifyBone_Title", "{ControllerDescription} - Bone: {BoneName}"), Args);
	}
	else
	{
		return FText::Format(LOCTEXT("AnimGraphNode_ModifyBone_Title", "{ControllerDescription}\nBone: {BoneName}"), Args);
	}
}

FString UAnimGraphNode_RotationMultiplier::GetNodeNativeTitle(ENodeTitleType::Type TitleType) const
{
	// Do not setup this function for localization, intentionally left unlocalized!
	FString Result = GetControllerDescription().ToString();
	Result += (TitleType == ENodeTitleType::ListView) ? TEXT(" - ") : TEXT("\n");
	Result += FString::Printf(TEXT("Bone: %s"), *Node.TargetBone.BoneName.ToString());
	return Result;
}

#undef LOCTEXT_NAMESPACE
