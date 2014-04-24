// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimationGraphSchema.h"
#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "AnimGraphNode_LocalToComponentSpace.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_LocalToComponentSpace

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_LocalToComponentSpace::UAnimGraphNode_LocalToComponentSpace(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FLinearColor UAnimGraphNode_LocalToComponentSpace::GetNodeTitleColor() const
{
	return FLinearColor(0.7f, 0.7f, 0.7f);
}

FString UAnimGraphNode_LocalToComponentSpace::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_LocalToComponentSpace_Tooltip", "Convert Local Pose to Component Space Pose").ToString();
}

FText UAnimGraphNode_LocalToComponentSpace::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("AnimGraphNode_LocalToComponentSpace_Title", "Local To Component");
}

FString UAnimGraphNode_LocalToComponentSpace::GetNodeCategory() const
{
	return TEXT("Convert Spaces");
}

void UAnimGraphNode_LocalToComponentSpace::CreateOutputPins()
{
	const UAnimationGraphSchema* Schema = GetDefault<UAnimationGraphSchema>();
	CreatePin(EGPD_Output, Schema->PC_Struct, TEXT(""), FComponentSpacePoseLink::StaticStruct(), /*bIsArray=*/ false, /*bIsReference=*/ false, TEXT("ComponentPose"));
}

void UAnimGraphNode_LocalToComponentSpace::PostProcessPinName(const UEdGraphPin* Pin, FString& DisplayName) const
{
	DisplayName = TEXT("");
}

#undef LOCTEXT_NAMESPACE