// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_CopyBone.h"

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

FText UAnimGraphNode_CopyBone::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_CopyBone_Tooltip", "The Copy Bone control copies the Transform data or any component of it - i.e. Translation, Rotation, or Scale - from one bone to another.");
}

FText UAnimGraphNode_CopyBone::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
	Args.Add(TEXT("SourceBoneName"), FText::FromName(Node.SourceBone.BoneName));
	Args.Add(TEXT("TargetBoneName"), FText::FromName(Node.TargetBone.BoneName));

	FText NodeTitle;
	if (TitleType == ENodeTitleType::ListView)
	{
		if ((Node.SourceBone.BoneName == NAME_None) && (Node.TargetBone.BoneName == NAME_None))
		{
			NodeTitle = FText::Format(LOCTEXT("AnimGraphNode_CopyBone_MenuTitle", "{ControllerDescription}"), Args);
		}
		else
		{
			NodeTitle = FText::Format(LOCTEXT("AnimGraphNode_CopyBone_ListTitle", "{ControllerDescription} - Source Bone: {SourceBoneName} - Target Bone: {TargetBoneName}"), Args);
		}
	}
	else
	{
		NodeTitle = FText::Format(LOCTEXT("AnimGraphNode_CopyBone_Title", "{ControllerDescription}\nSource Bone: {SourceBoneName}\nTarget Bone: {TargetBoneName}"), Args);
	}
	return NodeTitle;
}

#undef LOCTEXT_NAMESPACE
