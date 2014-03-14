// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"

UBehaviorTreeGraphNode_Service::UBehaviorTreeGraphNode_Service(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UBehaviorTreeGraphNode_Service::AllocateDefaultPins()
{
	//No Pins for services
}


FString UBehaviorTreeGraphNode_Service::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UBTService* Service = Cast<UBTService>(NodeInstance);
	if (Service != NULL)
	{
		return Service->GetNodeName();
	}

	return Super::GetNodeTitle(TitleType);
}

FName UBehaviorTreeGraphNode_Service::GetNameIcon() const
{
	return FName("BTEditor.Graph.BTNode.Service.Icon");
}

bool UBehaviorTreeGraphNode_Service::IsSubNode() const
{
	return true;
}
