// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_HandIKRetargeting.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_CopyBoneSkeletalControl

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_HandIKRetargeting::UAnimGraphNode_HandIKRetargeting(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

FText UAnimGraphNode_HandIKRetargeting::GetControllerDescription() const
{
	return LOCTEXT("HandIKRetargeting", "Hand IK Retargeting");
}

FString UAnimGraphNode_HandIKRetargeting::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_HandIKRetargeting_Tooltip", "Handle re-targeting of IK Bone chain. Moves IK bone chain to meet FK hand bones, based on HandFKWeight (to favor either side). (0 = favor left hand, 1 = favor right hand, 0.5 = equal weight).").ToString();
}

FText UAnimGraphNode_HandIKRetargeting::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
	return FText::Format(LOCTEXT("AnimGraphNode_CopyBone_Title", "{ControllerDescription}"), Args);
}

#undef LOCTEXT_NAMESPACE
