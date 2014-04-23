// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_Slot

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_Slot::UAnimGraphNode_Slot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FLinearColor UAnimGraphNode_Slot::GetNodeTitleColor() const
{
	return FLinearColor(0.7f, 0.7f, 0.7f);
}

FString UAnimGraphNode_Slot::GetTooltip() const
{
	return TEXT("Plays animation from code using AnimMontage");
}

FText UAnimGraphNode_Slot::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const FText SlotName = (Node.SlotName != NAME_None) ? FText::FromName(Node.SlotName) : LOCTEXT("NoSlotName", "(No slot name)");

	FFormatNamedArguments Args;
	Args.Add(TEXT("SlotName"), SlotName);

	if (TitleType == ENodeTitleType::ListView)
	{
		return FText::Format(LOCTEXT("SlotNodeListTitle", "Slot '{SlotName}'"), Args);
	}
	else
	{
		return FText::Format(LOCTEXT("SlotNodeTitle", "{SlotName}\nSlot"), Args);
	}
}

FString UAnimGraphNode_Slot::GetNodeNativeTitle(ENodeTitleType::Type TitleType) const
{
	// Do not setup this function for localization, intentionally left unlocalized!
	const FString SlotName = (Node.SlotName != NAME_None) ? Node.SlotName.ToString() : TEXT("(No slot name)");

	if (TitleType == ENodeTitleType::ListView)
	{
		return FString::Printf(TEXT("Slot '%s'"), *SlotName);
	}
	else
	{
		return FString::Printf(TEXT("%s\nSlot"), *SlotName);
	}
}

FString UAnimGraphNode_Slot::GetNodeCategory() const
{
	return TEXT("Blends");
}

#undef LOCTEXT_NAMESPACE
