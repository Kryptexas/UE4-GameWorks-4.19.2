// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_ModifyBone.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_ModifyBone

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_ModifyBone::UAnimGraphNode_ModifyBone(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FText UAnimGraphNode_ModifyBone::GetControllerDescription() const
{
	return LOCTEXT("TransformModifyBone", "Transform (Modify) Bone");
}

FString UAnimGraphNode_ModifyBone::GetKeywords() const
{
	return TEXT("Modify, Transform");
}

FString UAnimGraphNode_ModifyBone::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_ModifyBone_Tooltip", "The Transform Bone node alters the transform - i.e. Translation, Rotation, or Scale - of the bone").ToString();
}

FText UAnimGraphNode_ModifyBone::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
	Args.Add(TEXT("BoneName"), FText::FromName(Node.BoneToModify.BoneName));

	if(TitleType == ENodeTitleType::ListView)
	{
		return FText::Format(LOCTEXT("AnimGraphNode_ModifyBone_Title", "{ControllerDescription} - Bone: {BoneName}"), Args);
	}
	else
	{
		return FText::Format(LOCTEXT("AnimGraphNode_ModifyBone_Title", "{ControllerDescription}\nBone: {BoneName}"), Args);
	}
}

#undef LOCTEXT_NAMESPACE
