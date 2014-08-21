// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_Trail.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_TrailBone

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_Trail::UAnimGraphNode_Trail(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FText UAnimGraphNode_Trail::GetControllerDescription() const
{
	return LOCTEXT("TrailController", "Trail controller");
}

FString UAnimGraphNode_Trail::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_Trail_Tooltip", "The Trail Controller.").ToString();
}

FText UAnimGraphNode_Trail::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
	Args.Add(TEXT("BoneName"), FText::FromName(Node.TrailBone.BoneName));

	if (TitleType == ENodeTitleType::ListView)
	{
		if (Node.TrailBone.BoneName == NAME_None)
		{
			return FText::Format(LOCTEXT("AnimGraphNode_Trail_MenuTitle", "{ControllerDescription}"), Args);
		}
		return FText::Format(LOCTEXT("AnimGraphNode_Trail_ListTitle", "{ControllerDescription} - Bone: {BoneName}"), Args);
	}
	else
	{
		return FText::Format(LOCTEXT("AnimGraphNode_Trail_Title", "{ControllerDescription}\nBone: {BoneName}"), Args);
	}
}

#undef LOCTEXT_NAMESPACE