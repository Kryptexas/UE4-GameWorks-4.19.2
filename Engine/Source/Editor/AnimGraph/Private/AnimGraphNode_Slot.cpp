// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_Slot

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

FString UAnimGraphNode_Slot::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
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
