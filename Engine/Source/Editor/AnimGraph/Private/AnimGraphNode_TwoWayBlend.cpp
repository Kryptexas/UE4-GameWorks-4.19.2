// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_TwoWayBlend

UAnimGraphNode_TwoWayBlend::UAnimGraphNode_TwoWayBlend(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_TwoWayBlend::GetNodeCategory() const
{
	return TEXT("Blends");
}

FLinearColor UAnimGraphNode_TwoWayBlend::GetNodeTitleColor() const
{
	return FLinearColor(0.75f, 0.75f, 0.75f);
}

FString UAnimGraphNode_TwoWayBlend::GetTooltip() const
{
	return TEXT("Blend two poses together");
}

FString UAnimGraphNode_TwoWayBlend::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return TEXT("Blend");
}
