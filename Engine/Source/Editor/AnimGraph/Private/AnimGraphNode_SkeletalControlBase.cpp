// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimationGraphSchema.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()

/////////////////////////////////////////////////////
// UAnimGraphNode_SkeletalControlBase

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_SkeletalControlBase::UAnimGraphNode_SkeletalControlBase(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FLinearColor UAnimGraphNode_SkeletalControlBase::GetNodeTitleColor() const
{
	return FLinearColor(0.75f, 0.75f, 0.10f);
}

FString UAnimGraphNode_SkeletalControlBase::GetNodeCategory() const
{
	return TEXT("Skeletal Controls");
}

FText UAnimGraphNode_SkeletalControlBase::GetControllerDescription() const
{
	return LOCTEXT("ImplementMe", "Implement me");
}

FString UAnimGraphNode_SkeletalControlBase::GetTooltip() const
{
	return GetControllerDescription().ToString();
}

void UAnimGraphNode_SkeletalControlBase::CreateOutputPins()
{
	const UAnimationGraphSchema* Schema = GetDefault<UAnimationGraphSchema>();
	CreatePin(EGPD_Output, Schema->PC_Struct, TEXT(""), FComponentSpacePoseLink::StaticStruct(), /*bIsArray=*/ false, /*bIsReference=*/ false, TEXT("Pose"));
}

void UAnimGraphNode_SkeletalControlBase::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UAnimGraphNode_Base* TemplateNode = NewObject<UAnimGraphNode_Base>(GetTransientPackage(), GetClass());

	FString Category = TemplateNode->GetNodeCategory();
	FText MenuDesc = GetControllerDescription();
	FString Tooltip = GetControllerDescription().ToString();
	FString Keywords = TemplateNode->GetKeywords();

	TSharedPtr<FEdGraphSchemaAction_K2NewNode> NodeAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, Category, MenuDesc, Tooltip, 0, Keywords);
	NodeAction->NodeTemplate = TemplateNode;
}

#undef LOCTEXT_NAMESPACE