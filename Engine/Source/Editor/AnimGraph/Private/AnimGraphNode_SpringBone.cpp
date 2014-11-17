// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_SpringBone.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_SpringBone

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_SpringBone::UAnimGraphNode_SpringBone(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FText UAnimGraphNode_SpringBone::GetControllerDescription() const
{
	return LOCTEXT("SpringController", "Spring controller");
}

FString UAnimGraphNode_SpringBone::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_SpringBone_Tooltip", "The Spring Controller applies a spring solver that can be used to limit how far a bone can stretch from its reference pose position and apply a force in the opposite direction.").ToString();
}

FText UAnimGraphNode_SpringBone::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
	Args.Add(TEXT("BoneName"), FText::FromName(Node.SpringBone.BoneName));

	if(TitleType == ENodeTitleType::ListView)
	{
		return FText::Format(LOCTEXT("AnimGraphNode_SpringBone_Title", "{ControllerDescription} - Bone: {BoneName}"), Args);
	}
	else
	{
		return FText::Format(LOCTEXT("AnimGraphNode_SpringBone_Title", "{ControllerDescription}\nBone: {BoneName}"), Args);
	}
}

FString UAnimGraphNode_SpringBone::GetNodeNativeTitle(ENodeTitleType::Type TitleType) const
{
	// Do not setup this function for localization, intentionally left unlocalized!
	FString Result = GetControllerDescription().ToString();
	Result += (TitleType == ENodeTitleType::ListView) ? TEXT(" - ") : TEXT("\n");
	Result += FString::Printf(TEXT("Bone: %s"), *Node.SpringBone.BoneName.ToString());
	return Result;
}

#undef LOCTEXT_NAMESPACE