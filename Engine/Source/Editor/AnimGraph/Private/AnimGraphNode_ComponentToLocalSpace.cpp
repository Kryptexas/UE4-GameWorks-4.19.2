// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_ComponentToLocalSpace

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_ComponentToLocalSpace::UAnimGraphNode_ComponentToLocalSpace(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FLinearColor UAnimGraphNode_ComponentToLocalSpace::GetNodeTitleColor() const
{
	return FLinearColor(0.7f, 0.7f, 0.7f);
}

FString UAnimGraphNode_ComponentToLocalSpace::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_ComponentToLocalSpace_Tooltip", "Convert Component Space Pose to Local Pose").ToString();
}

FText UAnimGraphNode_ComponentToLocalSpace::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AnimGraphNode_ComponentToLocalSpace_Title", "Component To Local");
}

FString UAnimGraphNode_ComponentToLocalSpace::GetNodeCategory() const
{
	return TEXT("Convert Spaces");
}

void UAnimGraphNode_ComponentToLocalSpace::PostProcessPinName(const UEdGraphPin* Pin, FString& DisplayName) const
{
	DisplayName = TEXT("");
}

#undef LOCTEXT_NAMESPACE
