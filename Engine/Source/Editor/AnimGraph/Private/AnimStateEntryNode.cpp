// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimStateNode.cpp
=============================================================================*/

#include "AnimGraphPrivatePCH.h"

#include "EdGraphUtilities.h"

/////////////////////////////////////////////////////
// UAnimStateEntryNode

UAnimStateEntryNode::UAnimStateEntryNode(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UAnimStateEntryNode::AllocateDefaultPins()
{
	const UAnimationStateMachineSchema* Schema = GetDefault<UAnimationStateMachineSchema>();
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, Schema->PC_Exec, TEXT(""), NULL, false, false, TEXT("Entry"));
}

FString UAnimStateEntryNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UEdGraph* Graph = GetGraph();
	return Graph->GetName();
}

FString UAnimStateEntryNode::GetTooltip() const
{
	return TEXT("Entry point for state machine");
}

UEdGraphNode* UAnimStateEntryNode::GetOutputNode() const
{
	if(Pins.Num() > 0 && Pins[0] != NULL)
	{
		check(Pins[0]->LinkedTo.Num() <= 1);
		if(Pins[0]->LinkedTo.Num() > 0 && Pins[0]->LinkedTo[0]->GetOwningNode() != NULL)
		{
			return Pins[0]->LinkedTo[0]->GetOwningNode();
		}
	}
	return NULL;
}


