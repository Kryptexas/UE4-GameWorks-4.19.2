// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_BlendListByBool

UAnimGraphNode_BlendListByBool::UAnimGraphNode_BlendListByBool(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_BlendListByBool::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(TEXT("Blend Poses by bool"));
}

FString UAnimGraphNode_BlendListByBool::GetTooltip() const
{
	return FString::Printf(TEXT("Blend List (by bool)"));
}

void UAnimGraphNode_BlendListByBool::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const 
{
	FName BlendPoses(TEXT("BlendPose"));
	FName BlendTimes(TEXT("BlendTime"));

	if (ArrayIndex != INDEX_NONE)
	{
		// Note: This is intentionally flipped, as it looks better with true as the topmost element!
		FString TrueFalse = (ArrayIndex == 0) ? TEXT("True") : TEXT("False");

		if (SourcePropertyName == BlendPoses)
		{
			Pin->PinFriendlyName = TrueFalse + TEXT(" Pose");
		}
		else if (SourcePropertyName == BlendTimes)
		{
			Pin->PinFriendlyName = TrueFalse + TEXT(" Blend Time");
		}
	}
}