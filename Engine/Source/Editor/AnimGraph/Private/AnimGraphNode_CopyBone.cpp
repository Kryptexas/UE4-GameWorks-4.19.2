// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_CopyBoneSkeletalControl

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_CopyBone::UAnimGraphNode_CopyBone(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FText UAnimGraphNode_CopyBone::GetControllerDescription() const
{
	return LOCTEXT("CopyBone", "Copy Bone");
}

FString UAnimGraphNode_CopyBone::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_CopyBone_Tooltip", "The Copy Bone control copies the Transform data or any component of it - i.e. Translation, Rotation, or Scale - from one bone to another.").ToString();
}

FText UAnimGraphNode_CopyBone::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
	Args.Add(TEXT("SourceBoneName"), FText::FromName(Node.SourceBone.BoneName));
	Args.Add(TEXT("TargetBoneName"), FText::FromName(Node.TargetBone.BoneName));
	return FText::Format(LOCTEXT("AnimGraphNode_CopyBone_Title", "{ControllerDescription}\nSource Bone: {SourceBoneName}\nTarget Bone: {TargetBoneName}"), Args);
}

FString UAnimGraphNode_CopyBone::GetNodeNativeTitle(ENodeTitleType::Type TitleType) const
{
	// Do not setup this function for localization, intentionally left unlocalized!
	return FString::Printf(TEXT("%s\nSource Bone: %s\nTarget Bone: %s"), *GetControllerDescription().ToString(), *Node.SourceBone.BoneName.ToString(), *Node.TargetBone.BoneName.ToString());
}

#undef LOCTEXT_NAMESPACE
