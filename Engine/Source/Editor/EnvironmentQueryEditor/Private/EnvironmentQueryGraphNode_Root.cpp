// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnvironmentQueryEditorPrivatePCH.h"

UEnvironmentQueryGraphNode_Root::UEnvironmentQueryGraphNode_Root(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

void UEnvironmentQueryGraphNode_Root::AllocateDefaultPins()
{
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, TEXT("Transition"), TEXT(""), NULL, false, false, TEXT("In"));
}

FString UEnvironmentQueryGraphNode_Root::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString("ROOT");
}
