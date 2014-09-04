// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "AnimGraphNode_Slot.h"

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

FText UAnimGraphNode_Slot::GetTooltipText() const
{
	return LOCTEXT("AnimSlotNode_Tooltip", "Plays animation from code using AnimMontage");
}

FText UAnimGraphNode_Slot::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (Node.SlotName != NAME_None)
	{
		if (TitleType == ENodeTitleType::ListView)
		{
			return LOCTEXT("SlotNodeListTitle_NoName", "Slot '(No slot name)'");
		}
		else
		{
			return LOCTEXT("SlotNodeTitle_NoName", "(No slot name)\nSlot");
		}
	}
	else if (!CachedNodeTitles.IsTitleCached(TitleType))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("SlotName"), FText::FromName(Node.SlotName));

		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("SlotNodeListTitle", "Slot '{SlotName}'"), Args));
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("SlotNodeTitle", "{SlotName}\nSlot"), Args));
		}
	}
	return CachedNodeTitles[TitleType];
}

FString UAnimGraphNode_Slot::GetNodeCategory() const
{
	return TEXT("Blends");
}

void UAnimGraphNode_Slot::BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog)
{
	UAnimBlueprint* AnimBlueprint = GetAnimBlueprint();
	if (AnimBlueprint->TargetSkeleton)
	{
		AnimBlueprint->TargetSkeleton->AddSlotNodeName(Node.SlotName);
		AnimBlueprint->TargetSkeleton->AddSlotGroupName(Node.GroupName);
	}
}

#undef LOCTEXT_NAMESPACE
